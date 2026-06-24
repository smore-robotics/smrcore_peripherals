> DIR: src/

# Overview

实现与公开头文件同处 `src/`。公开 API 在 `peripherals/`；协议与设备 IO 在 `protocols/`；线程间工具在 `common/`。

# Contents

- peripherals/: 基类 `Peripheral`、公共 DTO、`FtSensor`/`SpaceMouse` 门面，见 `peripherals/_index.md`
- protocols/: 串口/Linux input 协议实现，见 `protocols/_index.md`
- common/: 内部工具（不对外安装）
  - clock.hpp: `internal::NowSeconds()` 单调时钟 [s]
  - latest_slot.hpp: `LatestSlot<T>` 线程安全 consume-on-read 单槽
  - resource_path.hpp/cpp: `ResolveResourcePath()` 按可执行文件/环境变量 `SMRCORE_PERIPHERALS_CONFIG_DIR` 解析相对资源路径

> FLOW: 门面派生类管理线程 → 协议层读写/解析 → `LatestSlot` 写入 → `GetSample()` 非阻塞取最新值并清空槽
