#!/usr/bin/env python3
"""Build branded, standalone PDF/HTML manuals from repository Markdown."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

from assemble_manual import assemble

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output', nargs='?', type=Path, default=ROOT / 'dist')
    parser.add_argument('version', nargs='?', default='dev')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9.-]+', args.version):
        parser.error('invalid version')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as temporary:
        source = Path(temporary) / 'manual.md'
        for language in ('zh', 'en'):
            source.write_text(assemble(language))
            stem = output / f'smrcore_peripherals-manual-{language}-v{args.version}'
            subtitle = '外设接入与操作手册' if language == 'zh' else 'Integration & Operation Guide'
            toc = '目录' if language == 'zh' else 'Contents'
            common = ['pandoc', str(source), '--standalone', '--toc', '--toc-depth=2',
                      '--number-sections', '--metadata-file=docs/manual/style.yaml',
                      '--metadata', f'subtitle={subtitle}', '--metadata', f'date=Version {args.version}',
                      '--metadata', f'toc-title={toc}']
            subprocess.run(common + ['--self-contained', '--css', 'docs/manual/style.css',
                                     '-o', str(stem) + '.html'], cwd=ROOT, check=True)
            subprocess.run(common + ['--template', 'docs/manual/eisvogel.latex',
                                     '--pdf-engine=xelatex', '--include-in-header',
                                     'docs/manual/layout.tex', '-o', str(stem) + '.pdf'],
                           cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
