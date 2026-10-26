#!/usr/bin/env python3
"""Render release-specific bilingual notes from the canonical user guides."""
import argparse
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
DOCUMENTED_REPO = 'smore-robotics/smrcore_peripherals'


def render(tag, repository, sdk, commit):
    if not re.fullmatch(r'v(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)', tag):
        raise ValueError('tag must be vX.Y.Z')
    if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository):
        raise ValueError('repository must be owner/name')
    if not re.fullmatch(r'[0-9a-f]{40}', commit):
        raise ValueError('expected full source commit')
    sections = [f'# {tag}\n\nSource: `{commit}` · Robot SDK: `{sdk}`\n']
    for filename in ['docs/releases.md', 'docs/releases.en.md']:
        text = (ROOT / filename).read_text()
        if f'**{sdk}**' not in text:
            raise ValueError(f'{filename} must document pinned SDK {sdk}')
        text = text.replace(DOCUMENTED_REPO, repository)
        text = text.replace('/blob/HEAD/', f'/blob/{tag}/')
        text = text.replace('<version>', tag[1:])
        # Keep development/maintenance guidance on the website, not every release.
        text = text.split('## 版本识别')[0].split('## Version identity')[0]
        sections.append(text.strip())
    return '\n\n'.join(sections) + '\n'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tag', required=True)
    parser.add_argument('--repository', required=True)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    sdk = (ROOT / '.sdk-version').read_text().strip()
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    from version_history import history
    changes = '\n\n'.join(history(lang, args.tag, args.repository, latest_only=True) for lang in ('zh', 'en'))
    args.output.write_text(changes + '\n\n' + render(args.tag, args.repository, sdk, commit))


if __name__ == '__main__':
    main()
