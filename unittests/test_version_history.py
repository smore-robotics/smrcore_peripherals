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
