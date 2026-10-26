#!/usr/bin/env python3
"""Build version history from reachable tags and auditable Git changes."""
import argparse
from collections import defaultdict
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
GROUPS = {
    'spacemouse': ('SpaceMouse', 'SpaceMouse'),
    'ft': ('六维力传感器', 'Force/torque sensor'),
    'pico': ('Pico', 'Pico'),
    'master': ('主从臂', 'Master–follower arms'),
    'python': ('Python 接口', 'Python interfaces'),
    'sdk': ('SDK 与构建发布', 'SDK, build and release'),
    'docs': ('文档', 'Documentation'),
    'other': ('公共接口与其他更新', 'Common interfaces and other changes'),
}


def git(*args, root=ROOT):
    return subprocess.check_output(['git', *args], cwd=root, text=True).strip()


def category(paths):
    text = '\n'.join(paths).lower()
    for needle, key in [('zhonglin', 'master'), ('pico', 'pico'), ('spacemouse', 'spacemouse'), ('ft_sensor', 'ft')]:
        if needle in text:
            return key
    if any(p.startswith('python/') for p in paths):
        return 'python'
    if any(p.startswith(('scripts/', '.github/', 'cmake/')) or p in ('.sdk-version', 'CMakeLists.txt', 'conanfile.py') for p in paths):
        return 'sdk'
    if paths and all(p.endswith(('.md', '.css', '.yml', '.yaml', '.latex', '.tex')) for p in paths):
        return 'docs'
    return 'other'


def boundaries(target='HEAD', root=ROOT):
    if git('rev-parse', '--is-shallow-repository', root=root) == 'true':
        raise RuntimeError('Version history requires full Git history: git fetch --unshallow --tags')
    tip = git('rev-parse', target, root=root)
    tags = []
    for tag in git('tag', '--merged', tip, root=root).splitlines():
        if re.fullmatch(r'v(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)', tag):
            sha = git('rev-list', '-n', '1', tag, root=root)
            distance = int(git('rev-list', '--count', f'{sha}..{tip}', root=root))
            tags.append((distance, tag, sha))
    tags.sort(key=lambda t: (t[0], t[1]))
    points = [] if tags and tags[0][2] == tip else [('Unreleased', tip)]
    # Skip tags on diverging ancestors; each baseline must precede its target.
    for _, tag, sha in tags:
        if points:
            result = subprocess.run(['git', 'merge-base', '--is-ancestor', sha, points[-1][1]], cwd=root)
            if result.returncode == 1:
                continue
            if result.returncode:
                raise RuntimeError('Unable to determine release ancestry')
            if sha == points[-1][1]:
                continue
        points.append((tag, sha))
    return points


def section(label, tip, base, language, repository, root=ROOT):
    zh = language == 'zh'
    title = ('未发布' if zh else 'Unreleased') if label == 'Unreleased' else label.removeprefix('v')
    lines = [f'## {title}', '']
    if base:
        baseline, sha = base
        url = f'https://github.com/{repository}/compare/{sha}...{tip}'
        lines += [f'相对 {baseline} 的累计变化。[完整差异]({url})。' if zh else f'Cumulative changes since {baseline}. [Full diff]({url}).', '']
        revision = f'{sha}..{tip}'
    else:
        revision = tip
        lines += ['首个版本的提交记录。' if zh else 'Commit history for the first release.', '']
    grouped = defaultdict(list)
    commits = git('rev-list', '--reverse', '--no-merges', revision, root=root).splitlines()
    for commit in commits:
        message = git('show', '-s', '--format=%B', commit, root=root)
        subject = message.splitlines()[0]
        note = re.search(r'^Release-note-' + language + r':\s*(.+)$', message, re.MULTILINE | re.IGNORECASE)
        text = note.group(1) if note else subject
        paths = git('diff-tree', '--root', '--no-commit-id', '--name-only', '-r', commit, root=root).splitlines()
        text = text.replace('<', '&lt;').replace('>', '&gt;').replace('[', '\\[').replace(']', '\\]')
        grouped['docs' if subject.startswith('docs:') else category(paths)].append(f'- {text} ([{commit[:7]}](https://github.com/{repository}/commit/{commit}))')
    if not grouped:
        lines += ['无新增提交。' if zh else 'No new commits.', '']
    for key, titles in GROUPS.items():
        if key in grouped:
            lines += [f'### {titles[0 if zh else 1]}', '', *grouped[key], '']
    return '\n'.join(lines)


def history(language='zh', target='HEAD', repository='zhanhui-li-smore/smrcore_peripherals', root=ROOT, latest_only=False):
    points = boundaries(target, root)
    lines = ['# 版本说明' if language == 'zh' else '# Release notes', '',
             '按 Git 版本范围自动整理；条目优先使用提交中的发布说明，未提供时使用原始提交标题。分类依据变更文件，具体行为以链接的代码差异为准。' if language == 'zh' else
             'Generated from Git version ranges. Entries use release-note trailers when supplied, otherwise original commit titles. Categories follow changed files; linked diffs provide the implementation details.', '']
    for i, (label, tip) in enumerate(points):
        lines.append(section(label, tip, points[i+1] if i+1 < len(points) else None, language, repository, root))
        if latest_only:
            break
    return '\n'.join(lines) + '\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--repository', default='zhanhui-li-smore/smrcore_peripherals')
    args = parser.parse_args()
    for lang, filename in [('zh', 'release-notes.zh.md'), ('en', 'release-notes.md')]:
        (ROOT / 'docs/site' / filename).write_text(history(lang, repository=args.repository))
