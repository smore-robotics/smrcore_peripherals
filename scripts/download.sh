#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"

GITHUB_SDK_REPO="${GITHUB_SDK_REPO:-smore-robotics/smrcore_sdk}"
GITHUB_PERIPHERALS_REPO="${GITHUB_PERIPHERALS_REPO:-smore-robotics/smrcore_peripherals}"

DEST_DIR="${DEST_DIR:-third_party}"
SDK_VERSION="${SDK_VERSION:-}"
PERIPHERALS_VERSION="${PERIPHERALS_VERSION:-}"
DOWNLOAD_SDK="ON"
DOWNLOAD_PERIPHERALS="ON"
PLATFORM="linux"
ARCH="x86_64"

usage() {
    cat <<'USAGE'
Usage: scripts/download.sh [options]

Options:
  --sdk-version V            smrcore_sdk version, default: .sdk-version
  --peripherals-version V    smrcore_peripherals version, default: .peripherals-version
  --release-tag TAG          GitHub release tag for both packages
  --sdk-release-tag TAG      GitHub release tag for smrcore_sdk
  --peripherals-release-tag TAG
  --dest-dir DIR             default: third_party
  --no-sdk                   only download smrcore_peripherals
  --no-peripherals           only download smrcore_sdk
  -h, --help                 show this help

Environment:
  SDK_VERSION, PERIPHERALS_VERSION
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --sdk-version) SDK_VERSION="$2"; shift 2 ;;
        --peripherals-version) PERIPHERALS_VERSION="$2"; shift 2 ;;
        --release-tag) SDK_RELEASE_TAG="$2"; PERIPHERALS_RELEASE_TAG="$2"; shift 2 ;;
        --sdk-release-tag) SDK_RELEASE_TAG="$2"; shift 2 ;;
        --peripherals-release-tag) PERIPHERALS_RELEASE_TAG="$2"; shift 2 ;;
        --dest-dir) DEST_DIR="$2"; shift 2 ;;
        --no-sdk) DOWNLOAD_SDK="OFF"; shift ;;
        --no-peripherals) DOWNLOAD_PERIPHERALS="OFF"; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "download: unknown argument: $1" >&2; exit 2 ;;
    esac
done

read_version_file() {
    local file="$1"
    if [[ -f "${file}" ]]; then
        tr -d '[:space:]' < "${file}"
    fi
}

normalize_version() {
    local name="$1"
    local version="$2"
    if [[ -z "${version}" ]]; then
        echo "download: ${name} version is empty" >&2
        exit 2
    fi
    if [[ "${version}" == "latest" ]]; then
        printf '%s' "${version}"
        return
    fi
    version="${version#v}"
    if ! printf '%s' "${version}" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        echo "download: ${name} version must be x.y.z, vx.y.z, or latest; got ${version}" >&2
        exit 2
    fi
    printf '%s' "${version}"
}

github_latest_tag() {
    local repo="$1"
    local latest_url
    latest_url="$(curl -Ls -o /dev/null -w '%{url_effective}' "https://github.com/${repo}/releases/latest")"
    printf '%s' "${latest_url##*/}"
}

asset_suffix() {
    local version="$1"
    if [[ "${version}" == "latest" ]]; then
        printf 'latest'
    else
        printf 'v%s' "${version#v}"
    fi
}

release_tag() {
    local version="$1"
    local override="$2"
    local repo="$3"
    if [[ -n "${override}" ]]; then
        printf '%s' "${override}"
    elif [[ "${version}" == "latest" ]]; then
        github_latest_tag "${repo}"
    else
        printf 'v%s' "${version#v}"
    fi
}

download_file() {
    local url="$1"
    local output="$2"
    shift 2
    mkdir -p "$(dirname "${output}")"
    echo "Downloading ${url}"
    curl --fail --location --show-error --retry 3 --retry-delay 2 "$@" --output "${output}" "${url}"
}

extract_archive() {
    local archive="$1"
    local target="$2"
    rm -rf "${target}"
    mkdir -p "${target}"
    tar -xf "${archive}" -C "${target}"
}

download_package() {
    local package="$1"
    local version="$2"
    local repo="$3"
    local filename="$4"
    local target="$5"
    local url tag archive

    archive="${ROOT_DIR}/artifacts/${filename}"
    if [[ "${package}" == "smrcore_sdk" ]]; then
        tag="$(release_tag "${version}" "${SDK_RELEASE_TAG:-}" "${repo}")"
    else
        tag="$(release_tag "${version}" "${PERIPHERALS_RELEASE_TAG:-}" "${repo}")"
    fi
    url="https://github.com/${repo}/releases/download/${tag}/${filename}"
    download_file "${url}" "${archive}"

    echo "Extracting ${filename} -> ${target}"
    extract_archive "${archive}" "${target}"
}

main() {
    cd "${ROOT_DIR}"

    if [[ -z "${SDK_VERSION}" ]]; then
        SDK_VERSION="$(read_version_file "${ROOT_DIR}/.sdk-version")"
    fi
    if [[ -z "${PERIPHERALS_VERSION}" ]]; then
        PERIPHERALS_VERSION="$(read_version_file "${ROOT_DIR}/.peripherals-version")"
    fi

    SDK_VERSION="$(normalize_version smrcore_sdk "${SDK_VERSION}")"
    PERIPHERALS_VERSION="$(normalize_version smrcore_peripherals "${PERIPHERALS_VERSION}")"

    if [[ "${DOWNLOAD_SDK}" == "ON" && "${SDK_VERSION}" == "latest" ]]; then
        SDK_RELEASE_TAG="${SDK_RELEASE_TAG:-$(github_latest_tag "${GITHUB_SDK_REPO}")}"
        SDK_VERSION="${SDK_RELEASE_TAG#v}"
    fi
    if [[ "${DOWNLOAD_PERIPHERALS}" == "ON" && "${PERIPHERALS_VERSION}" == "latest" ]]; then
        PERIPHERALS_RELEASE_TAG="${PERIPHERALS_RELEASE_TAG:-$(github_latest_tag "${GITHUB_PERIPHERALS_REPO}")}"
        PERIPHERALS_VERSION="${PERIPHERALS_RELEASE_TAG#v}"
    fi

    mkdir -p "${ROOT_DIR}/artifacts" "${ROOT_DIR}/${DEST_DIR}"

    if [[ "${DOWNLOAD_SDK}" == "ON" ]]; then
        local sdk_suffix sdk_filename
        sdk_suffix="$(asset_suffix "${SDK_VERSION}")"
        sdk_filename="smrcore_sdk-cpp-${PLATFORM}-${ARCH}-${sdk_suffix}.tar.gz"
        download_package "smrcore_sdk" "${SDK_VERSION}" "${GITHUB_SDK_REPO}" \
            "${sdk_filename}" "${ROOT_DIR}/${DEST_DIR}/smrcore_sdk"
    fi

    if [[ "${DOWNLOAD_PERIPHERALS}" == "ON" ]]; then
        local peripherals_suffix peripherals_filename
        peripherals_suffix="$(asset_suffix "${PERIPHERALS_VERSION}")"
        peripherals_filename="smrcore_peripherals-cpp-${PLATFORM}-${ARCH}-${peripherals_suffix}.tar.gz"
        download_package "smrcore_peripherals" "${PERIPHERALS_VERSION}" "${GITHUB_PERIPHERALS_REPO}" \
            "${peripherals_filename}" "${ROOT_DIR}/${DEST_DIR}/smrcore_peripherals"
    fi

    echo "Done: ${ROOT_DIR}/${DEST_DIR}"
}

main "$@"
