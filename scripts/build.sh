#!/bin/bash
set -e

TYPE="Release"
BUILD_TESTS="ON"
WITH_SDK="ON"
SDK_ROOT=""
ARCH="x86"
JOBS=""
DEPS_BUILD_TYPE="Release"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
PROFILE_DIR="$PROJECT_ROOT/infra/conan/profiles"
CMAKE_GENERATOR="${SMRCORE_PERIPHERALS_CMAKE_GENERATOR:-}"

SMRCORE_PERIPHERALS_IS_WINDOWS=0
case "$(uname -s 2>/dev/null)" in
    MINGW*|MSYS*|CYGWIN*)
        SMRCORE_PERIPHERALS_IS_WINDOWS=1
        ;;
esac
if [ "${OSTYPE:-}" = "msys" ] || [ "${OSTYPE:-}" = "cygwin" ]; then
    SMRCORE_PERIPHERALS_IS_WINDOWS=1
fi
if [ "$SMRCORE_PERIPHERALS_IS_WINDOWS" = 1 ]; then
    echo "smrcore_peripherals V1 暂不支持 Windows standalone 构建。" >&2
    exit 1
fi

parallel_jobs() {
    if [ -n "${NUMBER_OF_PROCESSORS:-}" ]; then
        echo "$NUMBER_OF_PROCESSORS"
    elif command -v nproc >/dev/null 2>&1; then
        nproc
    else
        echo 4
    fi
}

is_msvc_generator() {
    case "$1" in
        *"Visual Studio"*) return 0 ;;
        *) return 1 ;;
    esac
}

show_help() {
    cat << EOF
用法: ./scripts/build.sh [选项]

选项:
  -t, --type TYPE       构建类型: Release (默认), Debug, RelWithDebInfo
  -a, --arch ARCH       目标架构: x86 (默认) 或 armv8；V1 暂不支持 Windows
  -j, --jobs N          并行编译核心数
      --with-sdk ON/OFF 构建 app_peripherals_bridge 等 SDK bridge 工具 (默认: ON)
      --sdk-root PATH   smrcore_sdk 安装树 (默认: third_party/prebuilt/smrcore_sdk)
      --tests ON/OFF    是否构建测试 (默认: ON)
  -G, --generator NAME  CMake 生成器 (默认: Ninja)
  -h, --help            显示帮助

示例:
  ./scripts/build.sh
  ./scripts/download.sh && ./scripts/build.sh
  ./scripts/build.sh --with-sdk OFF
  ./scripts/build.sh -t Debug --tests ON

Python 绑定请使用: ./scripts/build_py.sh
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) show_help; exit 0 ;;
        -t|--type) TYPE="$2"; shift 2 ;;
        -a|--arch) ARCH="$2"; shift 2 ;;
        -j|--jobs) JOBS="$2"; shift 2 ;;
        --with-sdk) WITH_SDK="$2"; shift 2 ;;
        --sdk-root) SDK_ROOT="$2"; shift 2 ;;
        --tests) BUILD_TESTS="$2"; shift 2 ;;
        -G|--generator) CMAKE_GENERATOR="$2"; shift 2 ;;
        *) echo "未知选项: $1" >&2; exit 1 ;;
    esac
done

if [[ "$ARCH" != "x86" && "$ARCH" != "armv8" ]]; then
    echo "不支持的架构: $ARCH" >&2
    exit 1
fi

if [[ "$BUILD_TESTS" != "ON" && "$BUILD_TESTS" != "OFF" ]]; then
    echo "--tests 只能是 ON 或 OFF" >&2
    exit 1
fi
if [[ "$WITH_SDK" != "ON" && "$WITH_SDK" != "OFF" ]]; then
    echo "--with-sdk 只能是 ON 或 OFF" >&2
    exit 1
fi
if [ -z "$SDK_ROOT" ]; then
    SDK_ROOT="$PROJECT_ROOT/third_party/prebuilt/smrcore_sdk"
fi

HOST_PROFILE="${CONAN_HOST_PROFILE:-}"
if [ -z "$HOST_PROFILE" ]; then
    if [ "$SMRCORE_PERIPHERALS_IS_WINDOWS" = 1 ]; then
        HOST_PROFILE="$PROFILE_DIR/win_bash"
    elif [ "$ARCH" = "armv8" ]; then
        HOST_PROFILE="$PROFILE_DIR/armv8"
    else
        HOST_PROFILE="$PROFILE_DIR/default"
    fi
fi
BUILD_PROFILE="${CONAN_BUILD_PROFILE:-}"
if [ -z "$BUILD_PROFILE" ]; then
    if [ "$SMRCORE_PERIPHERALS_IS_WINDOWS" = 1 ]; then
        BUILD_PROFILE="$PROFILE_DIR/win_bash"
    else
        BUILD_PROFILE="$PROFILE_DIR/default"
    fi
fi
if [ -z "$CMAKE_GENERATOR" ]; then
    if [ "$SMRCORE_PERIPHERALS_IS_WINDOWS" = 1 ]; then
        CMAKE_GENERATOR="Visual Studio 17 2022"
    else
        CMAKE_GENERATOR="Ninja"
    fi
fi
if [ -z "$JOBS" ]; then
    JOBS="$(parallel_jobs)"
fi

# shellcheck source=/dev/null
source "${SCRIPT_DIR}/ci/resolve_version"

echo "========================================"
echo "smrcore_peripherals 构建配置"
echo "========================================"
echo "平台: $([ "$SMRCORE_PERIPHERALS_IS_WINDOWS" = 1 ] && echo Windows || echo Unix-like)"
echo "构建类型: $TYPE"
echo "目标架构: $ARCH"
echo "构建测试: $BUILD_TESTS"
echo "SDK bridge: $WITH_SDK"
if [ "$WITH_SDK" = "ON" ]; then
    echo "SDK root: $SDK_ROOT"
fi
echo "并行编译: $JOBS"
echo "CMake 生成器: $CMAKE_GENERATOR"
echo "Conan host profile: $HOST_PROFILE"
echo "Conan build profile: $BUILD_PROFILE"
echo "版本: $SMRCORE_PERIPHERALS_CPP_VERSION (channel=$SMRCORE_PERIPHERALS_CHANNEL)"
echo "========================================"

BUILD_DIR="$PROJECT_ROOT/build_$TYPE"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

conan install "$PROJECT_ROOT" \
    --output-folder=. \
    --build=missing \
    -pr:b="$BUILD_PROFILE" \
    -pr:h="$HOST_PROFILE" \
    -s:b build_type="$DEPS_BUILD_TYPE" \
    -s:h build_type="$DEPS_BUILD_TYPE" \
    -o "&:build_tests=$([ "$BUILD_TESTS" = "ON" ] && echo True || echo False)"

if [ "$SMRCORE_PERIPHERALS_IS_WINDOWS" = 1 ]; then
    CONAN_GEN_DIR="$BUILD_DIR/build/generators"
else
    CONAN_GEN_DIR="$BUILD_DIR/build/$DEPS_BUILD_TYPE/generators"
fi
CONAN_TOOLCHAIN="$CONAN_GEN_DIR/conan_toolchain.cmake"
if [ ! -f "$CONAN_TOOLCHAIN" ]; then
    echo "未找到 Conan toolchain: $CONAN_TOOLCHAIN" >&2
    exit 1
fi

if [ -f "$CONAN_GEN_DIR/conanbuild.sh" ]; then
    # shellcheck source=/dev/null
    source "$CONAN_GEN_DIR/conanbuild.sh"
fi
export PATH="$CONAN_GEN_DIR:${PATH:-}"

CMAKE_ARGS=(
    "$PROJECT_ROOT"
    -G "$CMAKE_GENERATOR" \
    -DCMAKE_TOOLCHAIN_FILE="$CONAN_TOOLCHAIN" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/install" \
    -DBUILD_TESTS="$BUILD_TESTS" \
    -DSMR_PERIPHERAL_WITH_SDK="$WITH_SDK" \
    -DSMR_PERIPHERAL_SDK_ROOT="$SDK_ROOT" \
    -DSMRCORE_PERIPHERALS_VERSION="$SMRCORE_PERIPHERALS_CPP_VERSION"
)
if is_msvc_generator "$CMAKE_GENERATOR"; then
    CMAKE_ARGS+=(-A x64)
else
    CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE="$TYPE")
fi

cmake "${CMAKE_ARGS[@]}"

cmake --build . --config "$TYPE" --parallel "$JOBS"
cmake --install . --config "$TYPE"

echo "✓ 构建完成: $BUILD_DIR/install"

if [ "$BUILD_TESTS" = "ON" ]; then
    cat << EOF
运行测试:
  ./scripts/run_tests.sh -t $TYPE
  ./scripts/run_tests.sh -t $TYPE ut_<测试名>

构建 Python 绑定 (需先完成本脚本):
  ./scripts/build_py.sh
  ./scripts/run_test_py.sh -t $TYPE
EOF
fi
