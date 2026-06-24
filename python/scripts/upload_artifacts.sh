#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${SCRIPT_DIR}/ci/detect_version"

DIST_DIR="${REPO_ROOT}/dist"
WHEEL_COUNT="$(find "${DIST_DIR}" -maxdepth 1 -type f -name '*.whl' | wc -l)"
if [ "${WHEEL_COUNT}" -ne 1 ]; then
    echo "upload_artifacts: expected exactly 1 wheel in ${DIST_DIR}, found ${WHEEL_COUNT}" >&2
    exit 1
fi
WHEEL_FILE="$(find "${DIST_DIR}" -maxdepth 1 -type f -name '*.whl' | head -n 1)"

FILE_NAME="$(basename "${WHEEL_FILE}")"
# 与 C++ 制品同前缀 smrcore_peripherals/<x.y.z|latest>/；wheel 文件名保留 rcore 前缀与版本号
# （对齐 rcore_sdk_py-0.0.7-cp310-cp310-linux_x86_64.whl 命名风格）
REMOTE_PATH="smrcore_peripherals/${SMRCORE_PERIPHERALS_UPLOAD_VERSION}/${FILE_NAME}"

"${SCRIPT_DIR}/ci/upload-generic1.sh" "${WHEEL_FILE}" "${REMOTE_PATH}"
