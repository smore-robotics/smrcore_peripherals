#!/usr/bin/env bash
set -euo pipefail

GITHUB_SDK_REPO="${GITHUB_SDK_REPO:-smore-robotics/smrcore_sdk}"
GITHUB_PERIPHERALS_REPO="${GITHUB_PERIPHERALS_REPO:-smore-robotics/smrcore_peripherals}"
INSTALL_DIR="${INSTALL_DIR:-third_party}"

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd -P)"

DOWNLOAD_SDK="ON"
DOWNLOAD_PERIPHERALS="ON"

usage() {
    cat <<'USAGE'
Usage: scripts/download.sh [options]

Downloads released C++ packages from GitHub into third_party/.

By default reads .peripherals-version and .sdk-version (they may differ).
Override with PERIPHERALS_VERSION / SDK_VERSION, or VERSION for SDK only.

Options:
  --no-sdk            only download smrcore_peripherals
  --no-peripherals    only download smrcore_sdk
  -h, --help          show this help

Environment:
  PERIPHERALS_VERSION                override .peripherals-version
  SDK_VERSION, VERSION               override .sdk-version (VERSION aliases SDK)
  PERIPHERALS_RELEASE_TAG            default: v<PERIPHERALS_VERSION>
  SDK_RELEASE_TAG                    default: v<SDK_VERSION>; CI may use prerelease
  INSTALL_DIR                        default: third_party
  GITHUB_PERIPHERALS_REPO            default: smore-robotics/smrcore_peripherals
  GITHUB_SDK_REPO                    default: smore-robotics/smrcore_sdk
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-sdk) DOWNLOAD_SDK="OFF"; shift ;;
        --no-peripherals) DOWNLOAD_PERIPHERALS="OFF"; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "download: unknown argument: $1" >&2; exit 2 ;;
    esac
done

detect_platform() {
    case "$(uname -s)" in
        Linux*)  echo "linux" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *)
            echo "download: unsupported OS: $(uname -s)" >&2
            exit 1
            ;;
    esac
}

detect_arch() {
    case "$(uname -m)" in
        x86_64|amd64) echo "x86_64" ;;
        *)
            echo "download: unsupported architecture: $(uname -m)" >&2
            exit 1
            ;;
    esac
}

PLATFORM="$(detect_platform)"
ARCH="$(detect_arch)"

resolve_version() {
    local name="$1"
    local version="$2"
    local version_file="$3"
    local repo="$4"

    if [ -z "$version" ] && [ -f "$version_file" ]; then
        version="$(tr -d '[:space:]' < "$version_file")"
    fi
    if [ -z "$version" ]; then
        echo "download: set ${name}_VERSION or provide ${version_file##*/}" >&2
        exit 1
    fi
    if [ "$version" = "latest" ]; then
        local latest_url
        latest_url="$(curl -Ls -o /dev/null -w '%{url_effective}' "https://github.com/${repo}/releases/latest")"
        version="${latest_url##*/}"
    fi
    version="${version#v}"
    if ! printf '%s\n' "$version" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        echo "download: ${name} version must be x.y.z (or latest), got: ${version}" >&2
        exit 1
    fi
    printf '%s' "$version"
}

download_release() {
    local repo="$1"
    local release_tag="$2"
    local asset="$3"
    local target="$4"
    local url="https://github.com/${repo}/releases/download/${release_tag}/${asset}"

    echo "Downloading ${asset}"
    echo "  release: ${release_tag}"
    echo "  url:     ${url}"
    rm -f "${ROOT_DIR}/${asset}"
    curl -L --fail --show-error --retry 3 --retry-delay 2 "${url}" -o "${ROOT_DIR}/${asset}"

    echo "Extracting to ${target}"
    rm -rf "${ROOT_DIR}/${target}"
    mkdir -p "${ROOT_DIR}/${target}"
    tar -xf "${ROOT_DIR}/${asset}" -C "${ROOT_DIR}/${target}"
    rm -f "${ROOT_DIR}/${asset}"
}

cd "${ROOT_DIR}"

echo "Platform: ${PLATFORM}-${ARCH}"
echo "Install dir: ${ROOT_DIR}/${INSTALL_DIR}"

if [ "${DOWNLOAD_PERIPHERALS}" = "ON" ]; then
    PERIPHERALS_VERSION="$(resolve_version PERIPHERALS "${PERIPHERALS_VERSION:-}" \
        "${ROOT_DIR}/.peripherals-version" "${GITHUB_PERIPHERALS_REPO}")"
    PERIPHERALS_RELEASE_TAG="${PERIPHERALS_RELEASE_TAG:-v${PERIPHERALS_VERSION}}"
    download_release "${GITHUB_PERIPHERALS_REPO}" "${PERIPHERALS_RELEASE_TAG}" \
        "smrcore_peripherals-cpp-${PLATFORM}-${ARCH}-v${PERIPHERALS_VERSION}.tar.gz" \
        "${INSTALL_DIR}/smrcore_peripherals"
fi

if [ "${DOWNLOAD_SDK}" = "ON" ]; then
    SDK_VERSION="$(resolve_version SDK "${SDK_VERSION:-${VERSION:-}}" \
        "${ROOT_DIR}/.sdk-version" "${GITHUB_SDK_REPO}")"
    SDK_RELEASE_TAG="${SDK_RELEASE_TAG:-v${SDK_VERSION}}"
    download_release "${GITHUB_SDK_REPO}" "${SDK_RELEASE_TAG}" \
        "smrcore_sdk-cpp-${PLATFORM}-${ARCH}-v${SDK_VERSION}.tar.gz" \
        "${INSTALL_DIR}/smrcore_sdk"
fi

echo "Done."
if [ "${DOWNLOAD_PERIPHERALS}" = "ON" ]; then
    echo "  smrcore_peripherals ${PERIPHERALS_VERSION} -> ${INSTALL_DIR}/smrcore_peripherals"
fi
if [ "${DOWNLOAD_SDK}" = "ON" ]; then
    echo "  smrcore_sdk ${SDK_VERSION} -> ${INSTALL_DIR}/smrcore_sdk"
fi
