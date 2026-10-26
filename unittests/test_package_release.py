"""Check binary archives expose guides without internal publication tooling."""
import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location('package', Path(__file__).parents[1] / 'scripts/package_release.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


def test_user_documentation_allowlist(tmp_path, monkeypatch):
    monkeypatch.syspath_prepend(str(package.ROOT / "scripts"))
    package.copy_user_docs(tmp_path)
    assert (tmp_path / 'docs/site/pico.zh.md').is_file()
    assert (tmp_path / 'docs/releases.md').is_file()
    for path in ('maintainers', 'manual', 'release-baseline.json', 'reviewed-release-notes.json', 'pico-integration-status.md'):
        assert not (tmp_path / 'docs' / path).exists()
    assert not list(tmp_path.rglob('_index.md'))
