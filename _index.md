> DIR: smrcore_peripherals/

# Overview

公开外设 SDK 源码仓库：SpaceMouse 与六维力/力矩传感器的本地读取、协议解析、采样归一化及调试/桥接工具。不依赖 rcore 源码，不维护机器人 IDL；向控制器送数走 rcore SDK `Robot::Peripheral().UpdateSpaceMouseSample()` / `UpdateFtSensorSample()` 高频外部输入接口；标定流程保留在 rcore 主仓。

GitHub `smore-robotics/smrcore_peripherals` 是源码真源；rcore 主仓只作为可选 submodule 引用。

# Contents

- README.md: 仓库使用说明（配置、构建、示例应用）
- CMakeLists.txt: 顶层构建/安装/package config；`file(GLOB_RECURSE .../src/*.cpp CONFIGURE_DEPENDS)` 自动收集库源；`smrcore_peripherals` 设置 `EXPORT_NAME peripherals`，安装导出目标为 `smrcore::peripherals`；`install(DIRECTORY src/peripherals/ ... *.hpp)` 安装公开头（排除内部 `sample_model.hpp`）；`SMR_PERIPHERAL_WITH_SDK` 启用时通过同构建 target 或 `SMR_PERIPHERAL_SDK_ROOT` 下的 `smrcore_sdk` package 构建 bridge；产物 `build_<Type>/bin|lib|install`
- conanfile.py: Conan 2 包 `rcore-peripherals`；options `shared`/`build_tests`；版本来自 `SMRCORE_PERIPHERALS_VERSION`
- .gitlab-ci.yml: 旧 GitLab CI 兼容入口；默认 `--with-sdk OFF`，不依赖 standalone SDK 下载
- .github/workflows/ci.yml: GitHub Actions；覆盖无 SDK C++ 构建、下载 `.sdk-version` 指定 SDK 后构建 bridge、Python wheel
- .sdk-version: standalone `scripts/download.sh` 使用的 smrcore_sdk 默认版本
- cmake/: `find_package(smrcore_peripherals)` 模板，见 `cmake/_index.md`
- infra/conan/: Conan profile（x86/armv8 等）
- src/: 公开 API、协议层与内部工具，见 `src/_index.md`
- app/: 探测、读数、SDK 桥接与标定工具，见 `app/_index.md`
- python/: pybind11 wheel 绑定，见 `python/_index.md`；由 `scripts/build_py.sh` / `scripts/run_test_py.sh` 驱动
- scripts/: SDK 下载、C++ 构建、Python 构建、测试与打包，见 `scripts/_index.md`
- unittests/: 无硬件语义单测，见 `unittests/_index.md`

> FLOW: `*Options` 经 `Initialize()` 配置 → `Start()` 启后台线程 → `GetSample()` consume-on-read → 可选 SDK bridge 推送
