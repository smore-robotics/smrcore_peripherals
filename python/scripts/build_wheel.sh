#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

if [[ -z "${SMRCORE_PERIPHERALS_CPP_ROOT:-}" ]]; then
  echo "build_wheel: SMRCORE_PERIPHERALS_CPP_ROOT is required." >&2
  echo "请先运行仓库根目录: ./scripts/build.sh && ./scripts/build_py.sh" >&2
  exit 1
fi

inject_version="${SMRCORE_PERIPHERALS_VERSION:-${CI_COMMIT_TAG:-}}"
inject_version="${inject_version#v}"

if [[ -n "${inject_version}" ]]; then
  if [[ ! "${inject_version}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "build_wheel: version must be x.y.z or vx.y.z, got: ${inject_version}" >&2
    exit 1
  fi
  PYPROJECT="${ROOT_DIR}/pyproject.toml"
  VERSION_PY="${ROOT_DIR}/src/rcore_peripherals/_version.py"
  PYPROJECT_BAK="$(mktemp)"
  VERSION_PY_BAK="$(mktemp)"
  cp "${PYPROJECT}" "${PYPROJECT_BAK}"
  cp "${VERSION_PY}" "${VERSION_PY_BAK}"
  restore_version_files() {
    mv -f "${PYPROJECT_BAK}" "${PYPROJECT}"
    mv -f "${VERSION_PY_BAK}" "${VERSION_PY}"
  }
  trap restore_version_files EXIT
  echo "build_wheel: injecting wheel version ${inject_version}" >&2
  sed -i -E "s/^version = \".*\"/version = \"${inject_version}\"/" "${PYPROJECT}"
  printf '__version__ = "%s"\n' "${inject_version}" > "${VERSION_PY}"
  export SMRCORE_PERIPHERALS_VERSION="${inject_version}"
fi

rm -rf "${ROOT_DIR}/dist"
python3 -m pip wheel --no-deps -w dist .
