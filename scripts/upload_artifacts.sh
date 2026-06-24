#!/bin/bash

### ./upload_artifacts.sh [linux|windows]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
PROJECT_ROOT="${SCRIPT_DIR}/../"
BUILD_TYPE="${BUILD_TYPE:-Release}"

# 单一版本真源：导出 SMRCORE_PERIPHERALS_VERSION / SMRCORE_PERIPHERALS_CHANNEL。
# 非法 tag 在此处 return 1，配合 set -e 直接中止上传。
# shellcheck source=/dev/null
source "${SCRIPT_DIR}/ci/resolve_version"

detect_platform_name() {
    local uname_s
    uname_s="$(uname -s)"
    case "${uname_s}" in
        Linux) echo "linux" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *) echo "$(printf '%s' "${uname_s}" | tr '[:upper:]' '[:lower:]')" ;;
    esac
}

normalize_target() {
    local target="$1"
    case "${target}" in
        x86|linux|linux-x86|linux-x86_64) echo "linux" ;;
        windows|win|windows-x86|windows-x86_64) echo "windows" ;;
        *) echo "${target}" ;;
    esac
}

TARGET="$(normalize_target "${1:-${ARCH:-$(detect_platform_name)}}")"
if [ "${TARGET}" != "linux" ] && [ "${TARGET}" != "windows" ]; then
    echo "不支持的发布目标: ${TARGET}" >&2
    exit 1
fi

BUILD_DIR="${PROJECT_ROOT}/build_${BUILD_TYPE}"
INSTALL_DIR="${BUILD_DIR}/install"

# 制品文件名定稿（对齐 smrcore_sdk）：
#   release: smrcore_peripherals-cpp-<os>-x86_64-v<version>.tar.gz
#   latest : smrcore_peripherals-cpp-<os>-x86_64-latest.tar.gz（不带 v）
if [ "${SMRCORE_PERIPHERALS_CHANNEL}" = "release" ]; then
    TAR_VERSION_SUFFIX="v${SMRCORE_PERIPHERALS_VERSION}"
    UPLOAD_VERSION="${SMRCORE_PERIPHERALS_VERSION}"
else
    TAR_VERSION_SUFFIX="latest"
    UPLOAD_VERSION="latest"
fi
TAR_FILENAME="smrcore_peripherals-cpp-${TARGET}-x86_64-${TAR_VERSION_SUFFIX}.tar.gz"

if [ ! -d "${INSTALL_DIR}" ]; then
    echo "安装目录不存在: ${INSTALL_DIR}" >&2
    exit 1
fi

cd "${BUILD_DIR}"
tar -czf "${TAR_FILENAME}" -C "${INSTALL_DIR}" .

# 上传路径前缀统一为 smrcore_peripherals/<version|latest>/
"${PROJECT_ROOT}/python/scripts/ci/upload-generic1.sh" \
    "${TAR_FILENAME}" \
    "smrcore_peripherals/${UPLOAD_VERSION}/${TAR_FILENAME}"
