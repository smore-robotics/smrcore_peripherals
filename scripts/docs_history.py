"""Generate bilingual release pages before every MkDocs build."""
import importlib.util
import os
from pathlib import Path


def on_pre_build(config):
    script = Path(__file__).with_name('version_history.py')
    spec = importlib.util.spec_from_file_location('peripherals_history', script)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    repository = os.environ.get('GITHUB_REPOSITORY', 'zhanhui-li-smore/smrcore_peripherals')
    for language, filename in [('zh', 'release-notes.zh.md'), ('en', 'release-notes.md')]:
        (Path(config['docs_dir']) / filename).write_text(module.history(language, repository=repository))
