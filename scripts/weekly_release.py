#!/usr/bin/env python3
"""Plan a weekly patch release; emit metadata without modifying Git refs."""
import json
import os
import re
import subprocess
import urllib.error
import urllib.request


def git(*args):
    return subprocess.check_output(['git', *args], text=True).strip()


def plan():
    tags = [t for t in git('tag', '--list').splitlines()
            if re.fullmatch(r'v(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)', t)]
    if not tags:
        return 'v0.0.1', False
    latest = max(tags, key=lambda t: tuple(map(int, t[1:].split('.'))))
    head = git('rev-parse', 'HEAD')
    base = git('rev-parse', latest + '^{commit}')
    if base == head:
        return latest, True
    # Do not auto-release a rollback or a divergent branch as the next version.
    subprocess.run(['git', 'merge-base', '--is-ancestor', base, head], check=True)
    major, minor, patch = map(int, latest[1:].split('.'))
    return f'v{major}.{minor}.{patch + 1}', False


def published(tag):
    request = urllib.request.Request(
        f"{os.environ.get('GITHUB_API_URL', 'https://api.github.com')}/repos/"
        f"{os.environ['GITHUB_REPOSITORY']}/releases/tags/{tag}",
        headers={'Authorization': f"Bearer {os.environ['GH_TOKEN']}",
                 'Accept': 'application/vnd.github+json'})
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            release = json.load(response)
        return not release['draft'] and not release['prerelease']
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return False
        raise


def main():
    tag, existing = plan()
    skip = existing and published(tag)
    values = {'tag': '' if skip else tag, 'version': tag[1:],
              'build': 'false' if skip else 'true',
              'create_tag': 'false' if existing or skip else 'true'}
    with open(os.environ['GITHUB_OUTPUT'], 'a') as output:
        for key, value in values.items():
            print(f'{key}={value}', file=output)
    print(f'{"No changes; skipping" if skip else "Release candidate"}: {tag}')


if __name__ == '__main__':
    main()
