#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output="${1:-$root/dist}"
version="${2:-dev}"
[[ "$version" =~ ^[A-Za-z0-9.-]+$ ]] || { echo "Invalid version" >&2; exit 2; }
mkdir -p "$output"
output="$(cd "$output" && pwd)"
cd "$root"
for language in zh en; do
    if [[ "$language" == zh ]]; then
        inputs=(README.md app/README.md python/app/README.md docs/platform_setup.md python/TELEOP.md docs/releases.md docs/site/zhonglin.zh.md)
    else
        inputs=(README.en.md app/README.en.md python/app/README.en.md docs/platform_setup.en.md docs/releases.en.md docs/site/zhonglin.md)
    fi
    stem="$output/smrcore_peripherals-manual-$language-v$version"
    pandoc "${inputs[@]}" --standalone --toc --metadata title="Smartmore Peripherals $version" -o "$stem.html"
    pandoc "${inputs[@]}" --standalone --toc --pdf-engine=xelatex \
        -V mainfont="DejaVu Serif" -V CJKmainfont="Noto Sans CJK SC" -V geometry:margin=22mm \
        --metadata title="Smartmore Peripherals $version" -o "$stem.pdf"
done
