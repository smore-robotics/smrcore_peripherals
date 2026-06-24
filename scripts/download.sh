#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SDK_VERSION_FILE="$PROJECT_ROOT/.sdk-version"
SDK_ROOT="${SMRCORE_PERIPHERALS_SDK_ROOT:-$PROJECT_ROOT/third_party/smrcore_sdk}"
BASE_URL="${SMRCORE_SDK_DOWNLOAD_BASE_URL:-https://github.com/smore-robotics/smrcore_sdk/releases/download}"

show_help() {
    cat << EOF
用法: ./scripts/download.sh

读取 .sdk-version 下载 smrcore_sdk C++ 安装包并解压到:
  $SDK_ROOT

环境变量:
  SDK_VERSION, VERSION            覆盖 .sdk-version（须为 x.y.z）
  SDK_RELEASE_TAG                 下载使用的 release tag（默认 v<version>；CI 可用 prerelease）
  SMRCORE_PERIPHERALS_SDK_ROOT    覆盖解包目录
  SMRCORE_SDK_DOWNLOAD_BASE_URL   覆盖下载 base URL
EOF
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    show_help
    exit 0
fi
if [ "$#" -ne 0 ]; then
    echo "download.sh 不接受版本或架构参数；请修改 .sdk-version 或使用环境变量覆盖下载源。" >&2
    exit 1
fi

case "$(uname -s 2>/dev/null)" in
    Linux*) ;;
    MINGW*|MSYS*|CYGWIN*)
        echo "smrcore_peripherals V1 暂不支持 Windows 下载/构建。" >&2
        exit 1
        ;;
    *)
        echo "当前系统暂不支持: $(uname -s 2>/dev/null)" >&2
        exit 1
        ;;
esac

case "$(uname -m 2>/dev/null)" in
    x86_64|amd64)
        SDK_ARCH="x86_64"
        ;;
    aarch64|arm64)
        SDK_ARCH="aarch64"
        ;;
    *)
        echo "当前 CPU 架构暂不支持: $(uname -m 2>/dev/null)" >&2
        exit 1
        ;;
esac

SDK_VERSION="${SDK_VERSION:-${VERSION:-}}"
if [ -z "$SDK_VERSION" ] && [ -f "$SDK_VERSION_FILE" ]; then
    SDK_VERSION="$(tr -d '[:space:]' < "$SDK_VERSION_FILE")"
fi
if [ -z "$SDK_VERSION" ]; then
    echo "请设置 SDK_VERSION/VERSION 或在 .sdk-version 中指定版本。" >&2
    exit 1
fi

SDK_VERSION="${SDK_VERSION#v}"
if ! printf '%s\n' "$SDK_VERSION" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "SDK 版本须为 x.y.z，当前: $SDK_VERSION" >&2
    exit 1
fi

SDK_RELEASE_TAG="${SDK_RELEASE_TAG:-v$SDK_VERSION}"

TAR_NAME="smrcore_sdk-cpp-linux-${SDK_ARCH}-v${SDK_VERSION}.tar.gz"
URL="${BASE_URL}/${SDK_RELEASE_TAG}/${TAR_NAME}"
TMP_DIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

echo "下载 smrcore_sdk ${SDK_VERSION} (release: ${SDK_RELEASE_TAG})"
echo "  $URL"
if command -v curl >/dev/null 2>&1; then
    curl -fL "$URL" -o "$TMP_DIR/$TAR_NAME"
elif command -v wget >/dev/null 2>&1; then
    wget -O "$TMP_DIR/$TAR_NAME" "$URL"
else
    echo "需要 curl 或 wget 下载 SDK。" >&2
    exit 1
fi

rm -rf "$SDK_ROOT"
mkdir -p "$SDK_ROOT"
tar -xzf "$TMP_DIR/$TAR_NAME" -C "$SDK_ROOT" --strip-components=1

if [ ! -f "$SDK_ROOT/lib/cmake/smrcore_sdk/smrcore_sdkConfig.cmake" ]; then
    echo "SDK 解包后未找到 smrcore_sdkConfig.cmake: $SDK_ROOT" >&2
    exit 1
fi

echo "smrcore_sdk 已安装到: $SDK_ROOT"
