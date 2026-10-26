import importlib.util
from pathlib import Path
import sys

SCRIPTS = Path(__file__).parents[1] / 'scripts'
sys.path.insert(0,str(SCRIPTS))
spec = importlib.util.spec_from_file_location('local_notes',SCRIPTS/'local_codex_notes.py')
notes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(notes)


def test_subscription_environment_excludes_api_override(monkeypatch):
    monkeypatch.setenv('OPENAI_API_KEY','test-only')
    monkeypatch.setenv('CODEX_API_KEY','test-only')
    monkeypatch.setenv('OPENAI_BASE_URL','https://example.invalid')
    env=notes.subscription_env()
    assert 'OPENAI_API_KEY' not in env and 'CODEX_API_KEY' not in env
    assert 'OPENAI_BASE_URL' not in env
    assert env['GIT_TERMINAL_PROMPT']=='0'


def test_unchanged_and_failed_runs_do_not_consume_subscription():
    assert not notes.changed_since({'completed_sha':'a'},'a',100)
    assert not notes.changed_since({'retry_after':200},'b',100)
    assert notes.changed_since({'retry_after':200},'b',201)
    assert notes.changed_since({'completed_sha':'a'},'a',100,force=True)
