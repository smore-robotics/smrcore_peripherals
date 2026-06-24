#!/usr/bin/env bash
# rcore_peripherals Python 单元测试
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
PY_DIR="${PROJECT_ROOT}/python"
TYPE="Release"
CPP_ROOT=""
LIST_ONLY=0
TEST_TARGET=""

show_help() {
    cat <<'EOF'
用法: ./scripts/run_test_py.sh [选项] [pytest 目标]

先完成 C++ 构建与 Python wheel 构建，再安装 wheel 并运行 pytest。

选项:
  -t, --type TYPE       与 C++ 构建类型一致 (默认: Release)
      --cpp-root PATH   覆盖 SMRCORE_PERIPHERALS_CPP_ROOT
  -l, --list            仅列出可用测试并退出
  -h, --help            显示帮助

示例:
  ./scripts/build.sh
  ./scripts/build_py.sh
  ./scripts/run_test_py.sh
  ./scripts/run_test_py.sh -l
  ./scripts/run_test_py.sh tests/test_import.py
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) show_help; exit 0 ;;
        -t|--type) TYPE="$2"; shift 2 ;;
        --cpp-root) CPP_ROOT="$2"; shift 2 ;;
        -l|--list) LIST_ONLY=1; shift ;;
        -*) echo "未知选项: $1" >&2; exit 1 ;;
        *) TEST_TARGET="$1"; shift ;;
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

shopt -s nullglob
WHEELS=("${PY_DIR}"/dist/rcore_peripherals_py-*.whl)
shopt -u nullglob

if [[ "${#WHEELS[@]}" -eq 0 ]]; then
    echo "未找到 wheel: ${PY_DIR}/dist/rcore_peripherals_py-*.whl" >&2
    echo "请先运行: ./scripts/build_py.sh -t ${TYPE}" >&2
    exit 1
fi

if [[ "${#WHEELS[@]}" -ne 1 ]]; then
    echo "期望恰好 1 个 wheel，实际找到 ${#WHEELS[@]}: ${WHEELS[*]}" >&2
    exit 1
fi

python3 -m pip install -q --upgrade pip pytest
chmod +x "${PY_DIR}/scripts/"*.sh 2>/dev/null || true

cd "${PY_DIR}"
TEST_INSTALL_DIR="$(mktemp -d "${TMPDIR:-/tmp}/rcore_peripherals_pytest.XXXXXX")"
trap 'rm -rf "${TEST_INSTALL_DIR}"' EXIT

if [[ "${LIST_ONLY}" -eq 1 ]]; then
    python3 -m pip install --no-deps --target "${TEST_INSTALL_DIR}" "${WHEELS[0]}" >/dev/null
    PYTHONPATH="${TEST_INSTALL_DIR}" PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 python3 -m pytest --collect-only -q tests
    exit 0
fi

echo "========================================"
echo "rcore_peripherals Python 单元测试"
echo "========================================"
echo "wheel: ${WHEELS[0]}"
echo "========================================"

./scripts/verify_wheel.sh
python3 -m pip install --no-deps --target "${TEST_INSTALL_DIR}" "${WHEELS[0]}" >/dev/null

if [[ -n "${TEST_TARGET}" ]]; then
    PYTHONPATH="${TEST_INSTALL_DIR}" PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 python3 -m pytest "${TEST_TARGET}"
else
    PYTHONPATH="${TEST_INSTALL_DIR}" PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 python3 -m pytest tests
fi

echo "✓ Python 单元测试通过"
