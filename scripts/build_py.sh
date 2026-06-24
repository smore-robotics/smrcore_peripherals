#!/usr/bin/env bash
set -euo pipefail

TYPE="Release"
CPP_ROOT=""
INSTALL_WHEEL=1

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
PY_DIR="${PROJECT_ROOT}/python"

show_help() {
    cat <<'EOF'
用法: ./scripts/build_py.sh [选项]

先完成 C++ 构建（./scripts/build.sh），再编译 Python 绑定 wheel。

选项:
  -t, --type TYPE       与 C++ 构建类型一致 (默认: Release)
      --cpp-root PATH   覆盖 SMRCORE_PERIPHERALS_CPP_ROOT
      --no-install      只构建 wheel，不卸载/安装当前 Python 包
  -h, --help            显示帮助

示例:
  ./scripts/build.sh
  ./scripts/build_py.sh
  ./scripts/run_test_py.sh -t Release
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) show_help; exit 0 ;;
        -t|--type) TYPE="$2"; shift 2 ;;
        --cpp-root) CPP_ROOT="$2"; shift 2 ;;
        --no-install) INSTALL_WHEEL=0; shift ;;
        *) echo "未知选项: $1" >&2; exit 1 ;;
    esac
done

if [[ -z "${CPP_ROOT}" ]]; then
    CPP_ROOT="${PROJECT_ROOT}/build_${TYPE}/install"
fi

if [[ ! -f "${CPP_ROOT}/include/peripherals/peripherals.hpp" ]]; then
    echo "未找到已安装的 C++ SDK: ${CPP_ROOT}" >&2
    echo "请先运行: ./scripts/build.sh -t ${TYPE}" >&2
    exit 1
fi

if [[ ! -f "${CPP_ROOT}/lib/libsmrcore_peripherals.so" && ! -f "${CPP_ROOT}/bin/smrcore_peripherals.dll" ]]; then
    echo "未找到 libsmrcore_peripherals 运行时库于: ${CPP_ROOT}" >&2
    exit 1
fi

# shellcheck source=/dev/null
source "${SCRIPT_DIR}/ci/resolve_version"
export SMRCORE_PERIPHERALS_CPP_ROOT="${CPP_ROOT}"
export SMRCORE_PERIPHERALS_VERSION="${SMRCORE_PERIPHERALS_CPP_VERSION}"

echo "========================================"
echo "rcore_peripherals Python 绑定构建"
echo "========================================"
echo "C++ 安装目录: ${SMRCORE_PERIPHERALS_CPP_ROOT}"
echo "版本: ${SMRCORE_PERIPHERALS_VERSION}"
echo "========================================"

python3 -m pip install -q --upgrade pip
python3 -m pip install -q scikit-build-core pybind11 ninja wheel

chmod +x "${PY_DIR}/scripts/"*.sh "${PY_DIR}/scripts/ci/"*.sh 2>/dev/null || true

cd "${PY_DIR}"
./scripts/build_wheel.sh

if [[ "${INSTALL_WHEEL}" -eq 1 ]]; then
    shopt -s nullglob
    WHEELS=("${PY_DIR}"/dist/rcore_peripherals_py-*.whl)
    shopt -u nullglob

    if [[ "${#WHEELS[@]}" -ne 1 ]]; then
        echo "期望恰好 1 个 wheel，实际找到 ${#WHEELS[@]}: ${WHEELS[*]}" >&2
        exit 1
    fi

    echo "卸载当前 Python 包: rcore-peripherals-py"
    python3 -m pip uninstall -y rcore-peripherals-py >/dev/null 2>&1 || true
    python3 -m pip uninstall -y smrcore-peripherals-py >/dev/null 2>&1 || true

    echo "安装当前 wheel: ${WHEELS[0]}"
    python3 -m pip install --force-reinstall --no-deps "${WHEELS[0]}"
fi

echo "✓ Python 绑定构建完成: ${PY_DIR}/dist/"
