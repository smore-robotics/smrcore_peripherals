import importlib.util
from pathlib import Path
import subprocess

spec = importlib.util.spec_from_file_location('history', Path(__file__).parents[1] / 'scripts/version_history.py')
h = importlib.util.module_from_spec(spec)
spec.loader.exec_module(h)


def test_version_ranges_trailers_and_unreleased(tmp_path):
    def git(*args):
        return subprocess.check_output(['git', *args], cwd=tmp_path, text=True).strip()
    git('init', '-q')
    git('config', 'user.name', 'Test')
    git('config', 'user.email', 'test@example.com')
    def commit(path, message):
        file = tmp_path / path
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_text(message)
        git('add', '.')
        git('commit', '-qm', message)
    commit('README.md', 'Initial')
    git('tag', 'v0.0.1')
    commit('python/pico.py', 'feat: reader\n\nRelease-note-zh: Pico 支持读取姿态\nRelease-note-en: Read Pico poses')
    git('tag', 'v0.0.2')
    commit('docs/guide.md', 'docs: explain install')
    text = h.history('zh', root=tmp_path)
    assert text.index('## 未发布') < text.index('## 0.0.2') < text.index('## 0.0.1')
    latest = h.history('zh', target='v0.0.2', root=tmp_path, latest_only=True)
    assert 'Pico 支持读取姿态' in latest
    assert 'Initial' not in latest and 'explain install' not in latest
    assert '相对 v0.0.1' in latest and '### Pico' in latest
    assert 'Read Pico poses' in h.history('en', target='v0.0.2', root=tmp_path)
    # Unrelated branch tags must not become a release baseline.
    git('checkout', '-qb', 'other', 'v0.0.1')
    commit('x', 'unrelated')
    git('tag', 'v9.0.0')
    assert '9.0.0' not in h.history('zh', target='v0.0.2', root=tmp_path)


def test_categories():
    assert h.category(['python/app/zhonglin.py']) == 'master'
    assert h.category(['src/peripherals/ft_sensor/reader.cpp']) == 'ft'
    assert h.category(['.sdk-version']) == 'sdk'


def test_reviewed_notes_are_versioned_and_deduplicated(tmp_path):
    import json
    def git(*args):
        return subprocess.check_output(['git', *args], cwd=tmp_path, text=True).strip()
    git('init', '-q'); git('config', 'user.name', 'Test'); git('config', 'user.email', 'test@example.com')
    def commit(message):
        git('add', '.'); git('commit', '-qm', message)
        return git('rev-parse', 'HEAD')
    (tmp_path / 'x').write_text('one')
    base = commit('Initial'); git('tag', 'v0.0.1')
    (tmp_path / 'x').write_text('two')
    source = commit('raw source title')
    record = dict(base_sha=base, target_sha=source, commits=[source], groups=[dict(category='docs', entries=[dict(zh='审核摘要', en='Reviewed summary', commits=[source])])])
    docs = tmp_path / 'docs'; docs.mkdir()
    path = docs / 'reviewed-release-notes.json'
    path.write_text(json.dumps([record]))
    commit('docs: integrate notes')
    text = h.history('zh', root=tmp_path, latest_only=True)
    assert '审核摘要' in text and 'raw source title' not in text
    assert 'integrate notes' in text
    old = h.history('zh', target=source, root=tmp_path, latest_only=True)
    assert '审核摘要' not in old and 'raw source title' in old
    record['commits'] = [base]
    path.write_text(json.dumps([record])); commit('bad evidence')
    import pytest
    with pytest.raises(ValueError, match='Git range'):
        h.history('zh', root=tmp_path)


def test_user_baseline_hides_development_history(tmp_path):
    import json
    def git(*args):
        return subprocess.check_output(['git', *args], cwd=tmp_path, text=True).strip()
    git('init', '-q'); git('config', 'user.name', 'Test'); git('config', 'user.email', 'test@example.com')
    (tmp_path / 'x').write_text('old')
    git('add', '.'); git('commit', '-qm', 'experimental diary'); git('tag', 'v0.0.1')
    (tmp_path / 'docs').mkdir()
    (tmp_path / 'docs/release-baseline.json').write_text(json.dumps(dict(tag='v0.1.0', zh='四类外设', en='Four peripherals')))
    git('add', '.'); git('commit', '-qm', 'baseline'); git('tag', 'v0.1.0')
    assert '四类外设' in h.history(root=tmp_path)
    assert 'experimental diary' not in h.history(root=tmp_path)
    assert 'experimental diary' in h.history(target='v0.0.1', root=tmp_path)
    (tmp_path / 'x').write_text('next')
    git('add', '.'); git('commit', '-qm', 'internal cleanup')
    (tmp_path / 'x').write_text('new')
    git('add', '.'); git('commit', '-qm', 'feature\n\nRelease-note-zh: 新增读取\nRelease-note-en: New reader')
    git('tag', 'v0.1.1')
    text = h.history(root=tmp_path)
    assert '新增读取' in text and '四类外设' in text
    assert 'internal cleanup' not in text
    assert '四类外设' not in h.history(root=tmp_path, latest_only=True)
