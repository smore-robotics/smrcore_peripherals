> DIR: cmake/

# Overview

已安装 `smrcore_peripherals` 的 CMake package 配置模板，配合顶层 `write_basic_package_version_file` 生成 `ConfigVersion.cmake`。

# Contents

- PeripheralHelpers.cmake: `smr_peripheral_resolve_sdk_target(out_var)` 优先解析同构建 `rcoresdk` / `smrcore::sdk` / `rcore::sdk`，找不到时按 `SMR_PERIPHERAL_SDK_ROOT` 追加 `CMAKE_PREFIX_PATH` 并 `find_package(smrcore_sdk CONFIG REQUIRED)`；`smr_target_enable_sdk_bridge(target)` 为 bridge 应用注入 `SMR_PERIPHERAL_WITH_SDK` 并链接 SDK
- smrcore_peripheralsConfig.cmake.in: `@PACKAGE_INIT@` + `find_dependency(Threads)` + include 已安装的 `smrcore_peripheralsTargets.cmake`（导出目标 `smrcore::peripherals`）

用法：`find_package(smrcore_peripherals CONFIG)` → `target_link_libraries(... smrcore::peripherals)`
