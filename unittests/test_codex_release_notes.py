import importlib.util
import json
from pathlib import Path
import subprocess
import sys

import pytest

SCRIPTS = Path(__file__).parents[1] / 'scripts'
sys.path.insert(0, str(SCRIPTS))
spec = importlib.util.spec_from_file_location('codex_notes', SCRIPTS / 'codex_release_notes.py')
notes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(notes)


def test_published_baseline_ignores_unpublished_tag(tmp_path):
    def git(*args):
        return subprocess.check_output(['git', *args], cwd=tmp_path, text=True).strip()
    git('init', '-q')
    git('config', 'user.name', 'Test')
    git('config', 'user.email', 'test@example.com')
    (tmp_path / '.sdk-version').write_text('0.2.0\n')
    git('add', '.')
    git('commit', '-qm', 'base')
    git('tag', 'v0.0.1')
    (tmp_path / 'pico.txt').write_text('reader')
    git('add', '.')
    git('commit', '-qm', 'add reader')
    git('tag', 'v0.0.2')  # Build may have tagged before publication completed.
    target = git('rev-parse', 'HEAD')
    releases = [{'tag_name': 'v0.0.1'}, {'tag_name': 'v0.0.2', 'draft': True}]
    context = notes.prepare(tmp_path, tmp_path/'output', releases, 'owner/repo')
    assert context['base_tag'] == 'v0.0.1'
    assert context['target_sha'] == target
    assert context['commits'] == [target]
    assert '+reader' in (tmp_path/'output/changes.diff').read_text()
    assert 'add reader' in (tmp_path/'output/fallback-zh.md').read_text()


def sample():
    return {'groups': [{'category': 'pico', 'entries': [{'zh': '支持读取姿态。', 'en': 'Read poses.', 'commits': ['a'*40]}]}], 'uncertainties': []}


def test_render_bilingual_and_commit_links():
    context = {'commits':['a'*40], 'repository':'owner/repo', 'base_tag':'v1.0.0', 'target_sha':'a'*40, 'sdk_version':'0.2.0'}
    data = sample()
    notes.validate(data, context)
    assert '支持读取姿态' in notes.render(data, context, 'zh')
    assert 'Read poses' in notes.render(data, context, 'en')
    assert '/owner/repo/commit/'+ 'a'*40 in notes.render(data, context, 'en')


@pytest.mark.parametrize('mutation', ['unknown-commit', 'empty', 'wrong-category', 'missing-language'])
def test_reject_invalid_claim_evidence(mutation):
    data = sample()
    if mutation == 'unknown-commit':
        data['groups'][0]['entries'][0]['commits'] = ['b'*40]
    elif mutation == 'empty':
        data['groups'] = []
    elif mutation == 'wrong-category':
        data['groups'][0]['category'] = 'invented'
    else:
        del data['groups'][0]['entries'][0]['en']
    with pytest.raises(ValueError):
        notes.validate(data, {'commits': ['a'*40]})


def test_invalid_model_output_preserves_fallback(tmp_path, monkeypatch):
    source = tmp_path/'input'
    source.mkdir()
    context = {'target_sha':'a'*40, 'commits':['a'*40]}
    (source/'context.json').write_text(json.dumps(context))
    for lang in ('zh','en'):
        (source/f'fallback-{lang}.md').write_text('Git summary')
    monkeypatch.setenv('CODEX_FINAL_MESSAGE', 'not JSON')
    monkeypatch.setattr(sys, 'argv', ['notes', 'render', '--input', str(source), '--output', str(tmp_path/'out'), '--status', 'generated'])
    with pytest.raises(SystemExit):
        notes.main()
    assert json.loads((tmp_path/'out/context.json').read_text())['status'] == 'invalid-output'
    assert 'Git summary' in (tmp_path/'out/release-notes-zh.md').read_text()
