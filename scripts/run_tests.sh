#!/bin/bash
# smrcore_peripherals 单元测试运行脚本
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TYPE="Release"
LIST_ONLY=0
shopt -s nullglob

SMRCORE_PERIPHERALS_IS_WINDOWS=0
case "$(uname -s 2>/dev/null)" in
    MINGW*|MSYS*|CYGWIN*) SMRCORE_PERIPHERALS_IS_WINDOWS=1 ;;
esac

is_test_binary() {
    local f="$1"
    [ -f "$f" ] || return 1
    [ -x "$f" ] && return 0
    [ "$SMRCORE_PERIPHERALS_IS_WINDOWS" = 1 ] && [[ "$f" == *.exe ]] && return 0
    return 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--type) TYPE="$2"; shift 2 ;;
        -l|--list) LIST_ONLY=1; shift ;;
        -h|--help)
            echo "用法: ./scripts/run_tests.sh [-t Release|Debug] [-l] [测试名]"
            echo "  -l: 仅列出可用 ut_* 测试并退出"
            echo "  无参数: 运行所有 ut_* 测试"
            echo "  测试名: 运行指定测试"
            exit 0 ;;
        *) TEST_NAME="$1"; shift ;;
    esac
done

if [ -d "$SCRIPT_DIR/bin" ]; then
    BIN_DIR="$SCRIPT_DIR/bin"
    LIB_DIR="$SCRIPT_DIR/lib"
else
    BIN_DIR="$PROJECT_ROOT/build_${TYPE}/install/bin"
    LIB_DIR="$PROJECT_ROOT/build_${TYPE}/install/lib"
fi

[ ! -d "$BIN_DIR" ] && {
    echo "错误: 未找到 $BIN_DIR" >&2
    echo "请先运行: ./scripts/build.sh -t ${TYPE} --tests ON" >&2
    exit 1
}

if [ "$SMRCORE_PERIPHERALS_IS_WINDOWS" = 1 ]; then
    if [ -d "$LIB_DIR" ]; then
        export PATH="$LIB_DIR:$BIN_DIR:${PATH:-}"
    else
        export PATH="$BIN_DIR:${PATH:-}"
    fi
else
    [ -d "$LIB_DIR" ] && export LD_LIBRARY_PATH="$LIB_DIR:${LD_LIBRARY_PATH:-}"
fi

list_tests() {
    echo "可用单元测试 (ut_*)："
    local found=0
    for t in "$BIN_DIR"/ut_*; do
        is_test_binary "$t" || continue
        echo "  $(basename "$t")"
        found=1
    done
    [ $found -eq 0 ] && echo "  (无)"
}

if [ $LIST_ONLY -eq 1 ]; then
    list_tests
    exit 0
fi

run_one() {
    local exec="$BIN_DIR/$1"
    if [ ! -f "$exec" ] && [ -f "${exec}.exe" ]; then
        exec="${exec}.exe"
    fi
    [ ! -f "$exec" ] && { echo "错误: $1 不存在"; return 1; }

    cd "$PROJECT_ROOT"
    if [ "$SMRCORE_PERIPHERALS_IS_WINDOWS" = 1 ]; then
        "$exec"
        return $?
    fi
    local arch
    arch=$(file "$exec" | grep -o "ARM aarch64" || echo "")
    if [ -n "$arch" ] && [ "$(uname -m)" = "x86_64" ]; then
        if ! command -v qemu-aarch64-static &>/dev/null; then
            echo "错误: 检测到 ARM 可执行文件，但未找到 qemu-aarch64-static"
            return 1
        fi
        qemu-aarch64-static -L "${STAGING_DIR:-/}" "$exec"
        return $?
    fi
    "$exec"
}

if [ -n "${TEST_NAME:-}" ]; then
    run_one "$TEST_NAME"
    exit $?
fi

passed=0
failed=0
failed_list=""
for t in "$BIN_DIR"/ut_*; do
    is_test_binary "$t" || continue
    name=$(basename "$t")
    if run_one "$name"; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
        failed_list="$failed_list $name"
    fi
done

echo "========================================"
echo "通过: $passed, 失败: $failed"
[ $failed -gt 0 ] && echo "失败:$failed_list"
echo "========================================"
exit $failed
