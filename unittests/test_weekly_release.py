import importlib.util
from pathlib import Path
import subprocess

import pytest

spec = importlib.util.spec_from_file_location('weekly', Path(__file__).parents[1] / 'scripts/weekly_release.py')
weekly = importlib.util.module_from_spec(spec)
spec.loader.exec_module(weekly)


@pytest.fixture
def repo(tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)
    weekly.git('init', '-q')
    weekly.git('config', 'user.name', 'test')
    weekly.git('config', 'user.email', 'test@example.invalid')
    weekly.git('commit', '--allow-empty', '-qm', 'first')
    return tmp_path


def test_first_release_and_existing_tag(repo):
    assert weekly.plan() == ('v0.0.1', False)
    weekly.git('tag', 'v0.0.1')
    assert weekly.plan() == ('v0.0.1', True)


def test_patch_numeric_order_and_ignored_tags(repo):
    for tag in ['v0.0.9', 'v0.0.10', 'v1.0.0-rc1', 'other']:
        weekly.git('tag', tag)
    weekly.git('commit', '--allow-empty', '-qm', 'new')
    assert weekly.plan() == ('v0.0.11', False)


def test_rollback_refused(repo):
    old = weekly.git('rev-parse', 'HEAD')
    weekly.git('commit', '--allow-empty', '-qm', 'new')
    weekly.git('tag', 'v0.0.1')
    weekly.git('checkout', '--detach', old)
    with pytest.raises(subprocess.CalledProcessError):
        weekly.plan()


@pytest.mark.parametrize('released', [True, False])
def test_unchanged_release_skip_or_retry(repo, monkeypatch, released):
    weekly.git('tag', 'v0.0.1')
    output = repo / 'outputs'
    monkeypatch.setenv('GITHUB_OUTPUT', str(output))
    monkeypatch.setattr(weekly, 'published', lambda tag: released)
    weekly.main()
    data = dict(line.split('=', 1) for line in output.read_text().splitlines())
    assert data['build'] == ('false' if released else 'true')
    assert data['create_tag'] == 'false'
    assert data['tag'] == ('' if released else 'v0.0.1')
