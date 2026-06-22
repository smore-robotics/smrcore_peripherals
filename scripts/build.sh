#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
BUILD_DIR="${BUILD_DIR:-build}"
DEPS_DIR="${DEPS_DIR:-third_party}"
SDK_PREFIX="${SDK_PREFIX:-${ROOT_DIR}/${DEPS_DIR}/smrcore_sdk}"
PERIPHERALS_PREFIX="${PERIPHERALS_PREFIX:-${ROOT_DIR}/${DEPS_DIR}/smrcore_peripherals}"
BUILD_SDK_EXAMPLES="${SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES:-ON}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

usage() {
    cat <<'USAGE'
Usage: scripts/build.sh [options]

Options:
  --no-sdk-examples       build read-only peripheral examples only
  --build-dir DIR         default: build
  --build-type TYPE       default: Release
  -h, --help              show this help

Environment:
  SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=OFF
  SDK_PREFIX=/path/to/smrcore_sdk
  PERIPHERALS_PREFIX=/path/to/smrcore_peripherals
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-sdk-examples) BUILD_SDK_EXAMPLES="OFF"; shift ;;
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --build-type) BUILD_TYPE="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "build: unknown argument: $1" >&2; exit 2 ;;
    esac
done

if [[ ! -f "${PERIPHERALS_PREFIX}/lib/cmake/smrcore_peripherals/smrcore_peripheralsConfig.cmake" ]]; then
    echo "smrcore_peripherals artifact not found; running scripts/download.sh"
    download_args=()
    if [[ "${BUILD_SDK_EXAMPLES}" == "OFF" ]]; then
        download_args+=(--no-sdk)
    fi
    "${ROOT_DIR}/scripts/download.sh" "${download_args[@]}"
fi

if [[ "${BUILD_SDK_EXAMPLES}" == "ON" && ! -f "${SDK_PREFIX}/lib/cmake/smrcore_sdk/smrcore_sdkConfig.cmake" ]]; then
    echo "smrcore_sdk artifact not found; running scripts/download.sh --no-peripherals"
    "${ROOT_DIR}/scripts/download.sh" --no-peripherals
fi

prefix_path="${PERIPHERALS_PREFIX}"
if [[ "${BUILD_SDK_EXAMPLES}" == "ON" ]]; then
    prefix_path="${SDK_PREFIX};${PERIPHERALS_PREFIX}"
fi

cmake -S "${ROOT_DIR}" -B "${ROOT_DIR}/${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DSMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES="${BUILD_SDK_EXAMPLES}" \
    -DCMAKE_PREFIX_PATH="${prefix_path}"
cmake --build "${ROOT_DIR}/${BUILD_DIR}" --parallel

echo "Done: ${ROOT_DIR}/${BUILD_DIR}"
