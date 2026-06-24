#!/usr/bin/env bash
# Byte-compile every Python app script under python/app/ (recursively).
#
# This only checks that the scripts parse and compile; it does NOT run them and
# does NOT require hardware or the rcore_peripherals wheel to be installed.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd -P)"
cd "$ROOT_DIR"

find python/app -name '*.py' -print0 | xargs -0 python3 -m py_compile
echo "Python app scripts compile: ok"
