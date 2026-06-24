> DIR: scripts/

# Overview

仓库根目录构建与测试脚本。Python wheel 细节脚本在 `python/scripts/`。

# Contents

- download.sh: 读取仓库根 `.sdk-version`，按当前 Linux 架构下载 `smrcore_sdk` C++ 安装包并解压到 `third_party/prebuilt/smrcore_sdk`；不接受版本/架构参数，Windows 直接报错
- build.sh: C++ SDK 编译/安装（`build_<Type>/install`）；默认 `-a x86`、`--with-sdk ON`、`--tests ON`；`--sdk-root` 指向 standalone `smrcore_sdk` 安装树
- build_py.sh: 在 C++ 已安装前提下构建 Python wheel（调用 `python/scripts/build_wheel.sh`）
- run_tests.sh: C++ CTest（`unittests/`）
- run_test_py.sh: Python pytest（需先 `build_py.sh`；调用 `python/scripts/verify_wheel.sh`）
- package.sh: 打包 C++ install 目录为 `rcore_peripherals.tar.gz`
- upload_artifacts.sh: 将 `build_<Type>/install` 打成 `smrcore_peripherals-cpp-<os>-x86_64-<vX|latest>.tar.gz`，上传到 GitLab Generic Registry `smrcore_peripherals/<version|latest>/`
- ci/resolve_version: 从环境变量/Git 解析 `SMRCORE_PERIPHERALS_VERSION`（C++ 与 Python 共用）
