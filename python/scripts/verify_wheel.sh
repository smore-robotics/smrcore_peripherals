#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_DIR="${SMRCORE_PERIPHERALS_PY_VERIFY_TARGET:-$(mktemp -d)}"

shopt -s nullglob
WHEELS=("${ROOT_DIR}"/dist/rcore_peripherals_py-*.whl)
shopt -u nullglob
if [[ "${#WHEELS[@]}" -ne 1 ]]; then
  echo "verify_wheel: expected exactly 1 wheel, found ${#WHEELS[@]}: ${WHEELS[*]:-<none>}" >&2
  exit 1
fi

python3 -m pip install --no-deps --target "${TARGET_DIR}" "${WHEELS[0]}"
PYTHONPATH="${TARGET_DIR}" python3 - <<'PY'
from rcore_peripherals import FtSensor, SpaceMouse, _native

print(_native.linked_sdk())
print(FtSensor)
print(SpaceMouse)
PY
