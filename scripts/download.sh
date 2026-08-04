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
  SDK_RELEASE_TAG                 下载使用的 release tag（未设置时先 v<version>，不存在再 prerelease）
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

TAR_NAME="smrcore_sdk-cpp-linux-${SDK_ARCH}-v${SDK_VERSION}.tar.gz"
TMP_DIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

url_exists() {
    local url="$1"
    if command -v curl >/dev/null 2>&1; then
        curl -fsIL --head "$url" >/dev/null 2>&1
    elif command -v wget >/dev/null 2>&1; then
        wget --spider -q "$url" 2>/dev/null
    else
        echo "需要 curl 或 wget 下载 SDK。" >&2
        return 1
    fi
}

download_url() {
    local url="$1"
    local dest="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fL "$url" -o "$dest"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "$dest" "$url"
    else
        echo "需要 curl 或 wget 下载 SDK。" >&2
        return 1
    fi
}

if [ -n "${SDK_RELEASE_TAG:-}" ]; then
    RELEASE_TAGS=("$SDK_RELEASE_TAG")
else
    RELEASE_TAGS=("v$SDK_VERSION" "prerelease")
fi

SDK_RELEASE_TAG=""
URL=""
for tag in "${RELEASE_TAGS[@]}"; do
    candidate_url="${BASE_URL}/${tag}/${TAR_NAME}"
    if url_exists "$candidate_url"; then
        SDK_RELEASE_TAG="$tag"
        URL="$candidate_url"
        break
    fi
    echo "release ${tag} 未找到 ${TAR_NAME}，尝试下一个来源..." >&2
done

if [ -z "$SDK_RELEASE_TAG" ]; then
    echo "未找到 smrcore_sdk ${SDK_VERSION} 安装包。" >&2
    echo "已尝试 release tag: ${RELEASE_TAGS[*]}" >&2
    exit 1
fi

echo "下载 smrcore_sdk ${SDK_VERSION} (release: ${SDK_RELEASE_TAG})"
echo "  $URL"
download_url "$URL" "$TMP_DIR/$TAR_NAME"

rm -rf "$SDK_ROOT"
mkdir -p "$SDK_ROOT"
tar -xzf "$TMP_DIR/$TAR_NAME" -C "$SDK_ROOT" --strip-components=1

if [ ! -f "$SDK_ROOT/lib/cmake/smrcore_sdk/smrcore_sdkConfig.cmake" ]; then
    echo "SDK 解包后未找到 smrcore_sdkConfig.cmake: $SDK_ROOT" >&2
    exit 1
fi

echo "smrcore_sdk 已安装到: $SDK_ROOT"
