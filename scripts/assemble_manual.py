#!/usr/bin/env python3
"""Assemble chapters, namespace anchors and resolve repository document links."""
import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = 'https://zhanhui-li-smore.github.io/smrcore_peripherals/'


def assemble(language):
    zh = language == 'zh'
    sources = [
        ('概览与安装' if zh else 'Overview and installation', 'README.md' if zh else 'README.en.md'),
        ('C++ 示例' if zh else 'C++ examples', 'app/README.md' if zh else 'app/README.en.md'),
        ('Python 示例' if zh else 'Python examples', 'python/app/README.md' if zh else 'python/app/README.en.md'),
        ('平台配置' if zh else 'Platform setup', 'docs/platform_setup.md' if zh else 'docs/platform_setup.en.md'),
        ('Pico 双臂遥操' if zh else 'Pico dual-arm teleoperation', 'docs/site/pico.zh.md' if zh else 'docs/site/pico.md'),
        ('中凌主从臂' if zh else 'Zhonglin master arm', 'docs/site/zhonglin.zh.md' if zh else 'docs/site/zhonglin.md'),
        ('制品与版本' if zh else 'Artifacts and releases', 'docs/releases.md' if zh else 'docs/releases.en.md'),
    ]
    chapters = []
    for i, (title, source) in enumerate(sources, 1):
        text = (ROOT / source).read_text()
        text = re.sub(r'^# .+\n', '', text, count=1)
        text = re.sub(r'^(#{2,6}) \d+\.\s+', r'\1 ', text, flags=re.MULTILINE)
        if source.startswith('README'):
            text = re.sub(r'^.*(?:\[English\]|\[简体中文\]).*\n', '', text, flags=re.MULTILINE)
            text = re.sub(r'^---\s*$', '', text, count=1, flags=re.MULTILINE)
        # Links in concatenated documents no longer have their original base.
        def link(match):
            label, target = match.groups()
            if target.startswith(('https:', 'http:', 'mailto:')):
                return match.group(0)
            path, _, fragment = target.partition('#')
            if not path:
                return f'[{label}]({BASE}{"zh/" if zh else ""})'
            resolved = (ROOT / source).parent.joinpath(path).resolve()
            try:
                relative = resolved.relative_to(ROOT).as_posix()
            except ValueError:
                return match.group(0)
            return f'[{label}](https://github.com/zhanhui-li-smore/smrcore_peripherals/blob/HEAD/{relative}{"#" + fragment if fragment else ""})'
        text = re.sub(r'\[([^\]\n]+)\]\(([^\s)]+)\)', link, text)
        chapters.append(f'# {title} {{#chapter-{i}}}\n\n{text.strip()}\n')
    license_text = (ROOT / 'docs/manual/EISVOGEL-LICENSE').read_text()
    chapters.append('# ' + ('模板许可' if zh else 'Template license') + '\n\nEisvogel 2.4.0 — Pascal Wagler / John MacFarlane.\n\n' + license_text)
    return '\n'.join(chapters)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--language', choices=['zh', 'en'], required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(assemble(args.language))
