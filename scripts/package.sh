#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build_Release"
INSTALL_DIR="${ROOT_DIR}/package/rcore_peripherals"
"${ROOT_DIR}/scripts/build.sh" -t Release --tests OFF
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_DIR}"
tar -C "${ROOT_DIR}/package" -czf "${ROOT_DIR}/package/rcore_peripherals.tar.gz" rcore_peripherals
