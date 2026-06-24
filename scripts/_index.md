> DIR: scripts/

# Overview

仓库根目录构建与测试脚本。Python wheel 细节脚本在 `python/scripts/`。

# Contents

- download.sh: 读取 `.sdk-version`（或 `SDK_VERSION`/`VERSION`），按当前 Linux 架构下载 `smrcore_sdk-cpp-linux-<arch>-v<version>.tar.gz` 到 `third_party/smrcore_sdk`；`SDK_RELEASE_TAG` 默认 `v<version>`，CI 可用 `prerelease`；不接受 CLI 参数，Windows 直接报错
- build.sh: C++ SDK 编译/安装（`build_<Type>/install`）；默认 `-a x86`、`--with-sdk ON`、`--tests ON`；`--sdk-root` 指向 standalone `smrcore_sdk` 安装树
- build_py.sh: 在 C++ 已安装前提下构建 Python wheel（调用 `python/scripts/build_wheel.sh`）
- run_tests.sh: C++ CTest（`unittests/`）
- run_test_py.sh: Python pytest（需先 `build_py.sh`；调用 `python/scripts/verify_wheel.sh`）
- check_python_examples.sh: 递归 `py_compile` `examples/python/`（CI main-checks / PR smoke 共用）
- package.sh: 打包 C++ install 目录为 `rcore_peripherals.tar.gz`
- ci/resolve_version: 从环境变量/Git 解析 `SMRCORE_PERIPHERALS_VERSION`（C++ 与 Python 共用）
