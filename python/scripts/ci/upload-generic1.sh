#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"

source "${SCRIPT_DIR}/check_generic_token"
source "${SCRIPT_DIR}/gitlab_helper"

if [ $# -ne 2 ]; then
    echo "Usage: ./upload-generic1.sh local_file remote_path" >&2
    exit 1
fi

LOCAL_FILE="$1"
REMOTE_PATH="$2"

if [ ! -f "${LOCAL_FILE}" ]; then
    echo "Local file not found: ${LOCAL_FILE}" >&2
    exit 1
fi

echo "Uploading ${LOCAL_FILE} -> ${GENERIC_ARTIFACTS_URL}/${REMOTE_PATH}"

curl --location --header "${GENERIC_TOKEN_STR}" \
     --upload-file "${LOCAL_FILE}" \
     "${GENERIC_ARTIFACTS_URL}/${REMOTE_PATH}"
