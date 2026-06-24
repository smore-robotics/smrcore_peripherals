<div align="center">

# smrcore_peripherals

**从源码构建 SMRCore 外设库，探测、读取 SpaceMouse 与六维力传感器，并通过机器人 SDK 桥接到 rcore。**

[![License](https://img.shields.io/badge/License-Apache%202.0-1f6feb.svg)](LICENSE)

[English](README.en.md) · **简体中文**

</div>

---

`smrcore_peripherals` 是公开外设 SDK 源码仓库，包含 SpaceMouse 与六维力/力矩传感器的本地读取、协议解析、采样归一化、C++/Python API、调试工具和 SDK bridge app。

本仓库不依赖 rcore 源码，也不维护机器人 IDL。需要将外设数据送入机器人控制器时，通过 [`smrcore_sdk`](https://github.com/smore-robotics/smrcore_sdk) 的 `Robot::Peripheral().UpdateSpaceMouseSample()` / `UpdateFtSensorSample()` 高频外部输入接口完成，不走 RPC。

## 文档

| 文档 | 说明 |
|---|---|
| [C++ 示例](examples/cpp/README.md) | 当前源码内 app 与历史 examples 的构建和运行方式 |
| [Python 示例](examples/python/README.md) | Python wheel 构建、安装和脚本用法 |
| [平台配置](docs/platform_setup.md) | SpaceMouse 设备节点、串口权限、机器人连接 |

## 快速开始

仅构建外设库、基础 app 和单元测试，不需要机器人 SDK：

```bash
git clone https://github.com/smore-robotics/smrcore_peripherals.git
cd smrcore_peripherals

./scripts/build.sh --with-sdk OFF --tests ON
./scripts/run_tests.sh -t Release

./build_Release/bin/app_peripherals_probe
```

构建 SDK bridge app 需要先下载 `.sdk-version` 指定的 `smrcore_sdk`：

```bash
./scripts/download.sh
./scripts/build.sh --with-sdk ON --tests ON

./build_Release/bin/app_peripherals_bridge --robot <robot-ip>
```

## 主要功能

| 模块 | 说明 |
|---|---|
| SpaceMouse | 通过 Linux input 子系统读取原始事件，输出归一化 6-DOF + 夹爪目标状态 |
| F/T Sensor | 支持坤维 `kunwei_serial` 与鑫精诚 XJC `xjc_serial` 串口协议，输出六维力/力矩 |
| C++ API | `smrcore::peripherals` 命名空间，统一 `Initialize` → `Start` → `GetSample` → `Stop` → `Shutdown` 生命周期 |
| Python API | `rcore_peripherals` wheel，API 与 C++ 门面对齐 |
| SDK bridge | `app_peripherals_bridge` 通过 `smrcore_sdk` 向 rcore 注入 SpaceMouse / F/T sample |

## 仓库结构

| 路径 | 内容 |
|---|---|
| `src/` | 外设公开 API、内部工具和协议/设备 IO 实现 |
| `app/` | 当前推荐的 C++ 可执行工具：probe、read、bridge |
| `python/` | pybind11 Python wheel、示例脚本和测试 |
| `examples/` | 对外示例源码；与 `app/` 能力保持同语义，后续逐步收口 |
| `docs/` | 平台配置说明 |
| `scripts/download.sh` | 只下载 `smrcore_sdk`，不下载本仓库自身制品 |
| `scripts/build.sh` | 构建 C++ library、app、测试和可选 SDK bridge |

公开头文件安装到 `<peripherals/...>`，聚合入口：

```cpp
#include "peripherals/peripherals.hpp"
```

安装后外部项目可使用：

```cmake
find_package(smrcore_peripherals CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE smrcore::peripherals)
```

## 版本文件

| 文件 | 含义 |
|---|---|
| `.sdk-version` | standalone bridge 默认使用的 `smrcore_sdk` 版本 |

`scripts/download.sh` 读取 `.sdk-version`，自动按当前 Linux 架构下载 `smrcore_sdk-cpp-linux-<arch>-v<version>.tar.gz` 到 `third_party/smrcore_sdk/`。V1 暂不支持 Windows standalone 下载/构建。

## 常用命令

```bash
# 无 SDK bridge 的轻量构建
./scripts/build.sh --with-sdk OFF --tests ON

# 下载 SDK 并构建 bridge
./scripts/download.sh
./scripts/build.sh --with-sdk ON --tests ON

# 运行 C++ 单元测试
./scripts/run_tests.sh -t Release

# 构建并验证 Python wheel
./scripts/build_py.sh
./scripts/run_test_py.sh -t Release
```

## 安全提示

> 机器人是危险设备。运行任何 bridge 或运动相关示例前，请确认工作空间已清空、急停可触达，且外设输入不会导致非预期运动。F/T 与遥操作数据会直接影响控制器行为。

## 许可证

本仓库以 [Apache License 2.0](LICENSE) 发布。第三方依赖遵循其各自许可证。

<div align="center">

Copyright © Smartmore Corporation

</div>
