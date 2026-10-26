#!/usr/bin/env python3
"""Poll a trusted branch; use local ChatGPT auth to archive release-note drafts."""
import argparse
import fcntl
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

from codex_release_notes import prepare, render, validate

ROOT = Path(__file__).resolve().parents[1]


def run(args, cwd=None, env=None, timeout=120):
    return subprocess.check_output(args, cwd=cwd, env=env, text=True, stderr=subprocess.PIPE, timeout=timeout).strip()


def subscription_env():
    env = os.environ.copy()
    for name in ('OPENAI_API_KEY', 'CODEX_API_KEY', 'OPENAI_BASE_URL', 'OPENAI_API_BASE', 'CODEX_INTERNAL_ORIGINATOR_OVERRIDE'):
        env.pop(name, None)
    env['GIT_TERMINAL_PROMPT'] = '0'
    return env


def codex_path():
    binary = shutil.which('codex')
    if binary:
        return binary
    candidates = list(Path.home().glob('.cursor/extensions/openai.chatgpt-*/bin/linux-x86_64/codex'))
    candidates += list(Path.home().glob('.vscode/extensions/openai.chatgpt-*/bin/linux-x86_64/codex'))
    if not candidates:
        raise RuntimeError('Codex CLI not found; install it and sign in with ChatGPT')
    return str(max(candidates, key=lambda p: p.stat().st_mtime))


def changed_since(state, target, now, force=False):
    if force:
        return True
    if state.get('completed_sha') == target:
        return False
    return now >= state.get('retry_after', 0)


def archive(state_dir, repository, branch, context, data, env):
    checkout = state_dir / 'archive'
    checkout.mkdir(exist_ok=True)
    if not (checkout / '.git').exists():
        run(['git', 'init', '-q', str(checkout)])
        run(['git', 'remote', 'add', 'origin', f'https://github.com/{repository}.git'], checkout)
    remote = run(['git', 'ls-remote', '--heads', 'origin', 'refs/heads/'+branch], checkout, env)
    if remote:
        run(['git', 'fetch', 'origin', f'refs/heads/{branch}'], checkout, env)
        run(['git', 'checkout', '-B', branch, 'FETCH_HEAD'], checkout)
    elif run(['git', 'branch', '--list', branch], checkout):
        run(['git', 'checkout', branch], checkout)
    else:
        run(['git', 'checkout', '--orphan', branch], checkout)
    target = context['target_sha']
    destination = checkout / 'drafts' / target
    if (destination / 'context.json').exists():
        existing = json.loads((destination / 'context.json').read_text())
        if existing.get('base_sha') != context['base_sha']:
            raise ValueError('Existing draft has different baseline; refusing overwrite')
        run(['git', 'push', 'origin', f'HEAD:refs/heads/{branch}'], checkout, env)
        return
    destination.mkdir(parents=True, exist_ok=True)
    (destination / 'context.json').write_text(json.dumps({**context, 'status':'generated', 'auth':'local-chatgpt'}, indent=2)+'\n')
    (destination / 'draft.json').write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n')
    for lang in ('zh','en'):
        (destination/f'release-notes-{lang}.md').write_text(render(data, context, lang))
    (checkout/'README.md').write_text(f'# Codex 版本说明草稿\n\n本机 ChatGPT 登录生成，尚未人工审核。此分支仅保存文档，不含登录凭据。\n\n最新分析提交：`{target}`\n\n[中文草稿](drafts/{target}/release-notes-zh.md) · [English](drafts/{target}/release-notes-en.md)\n')
    run(['git', 'add', 'README.md', 'drafts/'+target], checkout)
    run(['git', '-c', 'user.name=Local Codex Notes', '-c', 'user.email=codex-notes@users.noreply.github.com',
         'commit', '-qm', f'docs: archive release notes for {target[:12]}'], checkout)
    run(['git', 'push', 'origin', f'HEAD:refs/heads/{branch}'], checkout, env)


def cycle(args):
    state_dir = args.state.expanduser().resolve()
    state_dir.mkdir(parents=True, exist_ok=True, mode=0o700)
    with open(state_dir/'lock', 'w') as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print('Another check is running; skipped')
            return
        state_file = state_dir/'status.json'
        state = json.loads(state_file.read_text()) if state_file.exists() else {}
        env = subscription_env()
        try:
            metadata = json.loads(run(['gh','api',f'repos/{args.repository}'], env=env))
            branch = metadata['default_branch']
            remote_url = f'https://github.com/{args.repository}.git'
            remote = run(['git','ls-remote',remote_url,'refs/heads/'+branch],env=env)
            target = remote.split()[0]
            if not changed_since(state, target, time.time(), args.force):
                print('Unchanged or waiting for retry; skipped')
                return
            binary = codex_path()
            # Status output goes to stderr on some CLI releases; never print auth files.
            auth = subprocess.run([binary,'login','status'],env=env,capture_output=True,text=True,timeout=30)
            if auth.returncode or 'logged in using chatgpt' not in (auth.stdout+auth.stderr).lower():
                raise RuntimeError('ChatGPT login required; run codex login locally (not API key auth)')
            source = state_dir/'source'
            if not (source/'.git').exists():
                run(['git','clone','--no-checkout',remote_url,str(source)],env=env)
            run(['git','fetch','origin',f'refs/heads/{branch}','--tags'],source,env)
            run(['git','checkout','--detach',target],source,env)
            # --jq .[] yields one JSON object per release; collect each page using jq.
            raw = run(['gh','api','--paginate',f'repos/{args.repository}/releases?per_page=100','--jq','.[]'],env=env)
            decoder = json.JSONDecoder()
            releases = []
            while raw.strip():
                release, end = decoder.raw_decode(raw.lstrip())
                releases.append(release)
                raw = raw.lstrip()[end:]
            input_dir = source/'.codex-release-input'
            context = prepare(source,input_dir,releases,args.repository)
            if not context['commits']:
                state.update(completed_sha=target, status='no-changes', retry_after=0)
            else:
                results = state_dir/'results'/target
                results.mkdir(parents=True, exist_ok=True)
                output = results/'draft.json'
                if output.exists():
                    try:
                        data = json.loads(output.read_text())
                        validate(data,context)
                    except (ValueError,TypeError,KeyError):
                        output.unlink()
                if not output.exists():
                    command = [binary,'--ask-for-approval','never','exec','--ignore-user-config',
                               '--sandbox','read-only','--ephemeral','--color','never',
                               '--output-schema',str(ROOT/'.github/codex/release-notes.schema.json'),
                               '--output-last-message',str(output),'-']
                    # No copied auth, no API fallback, no repo build or robot execution.
                    model_env = env.copy()
                    for name in ('GH_TOKEN', 'GITHUB_TOKEN'):
                        model_env.pop(name, None)
                    with open(results/'execution.log','w') as log:
                        completed = subprocess.run(command,cwd=source,env=model_env,
                            input=(ROOT/'.github/codex/release-notes.md').read_text(),
                            text=True,stdout=log,stderr=log,timeout=900)
                    if completed.returncode:
                        raise RuntimeError(f'Codex failed; inspect local log {results / "execution.log"}')
                data = json.loads(output.read_text())
                validate(data,context)
                archive(state_dir,args.repository,'automation/release-notes',context,data,env)
                state.update(completed_sha=target,status='generated',retry_after=0)
            state['checked_at'] = time.time()
            state_file.write_text(json.dumps(state,indent=2)+'\n')
            print(f'{state["status"]}: {target}')
        except Exception as error:
            state.update(status='failed',retry_after=time.time()+3600,checked_at=time.time())
            state_file.write_text(json.dumps(state,indent=2)+'\n')
            # Child stdout/stderr can contain credentials; do not echo CalledProcessError output.
            print(str(error) if isinstance(error,(RuntimeError,ValueError)) else type(error).__name__)
            raise SystemExit(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--repository',default='zhanhui-li-smore/smrcore_peripherals')
    parser.add_argument('--state',type=Path,default=Path.home()/'.local/share/smrcore-peripherals-notes')
    parser.add_argument('--force',action='store_true')
    cycle(parser.parse_args())
