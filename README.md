<div align="center">

# smrcore_peripherals

**基于 SMRCore 外设库，快速探测、读取 SpaceMouse 与六维力传感器，并桥接到机器人 SDK。**

[![License](https://img.shields.io/badge/License-Apache%202.0-1f6feb.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/smore-robotics/smrcore_peripherals?label=Release&color=1f6feb)](https://github.com/smore-robotics/smrcore_peripherals/releases)

[English](README.en.md) · **简体中文**

</div>

---

**SMRCore 外设库**的公开集成仓库，提供构建脚本、可直接运行的 C++ 与 Python
示例，以及预编译 C++ 包与 Python wheel 的下载助手。本仓库**不包含外设读写库
源码**（SpaceMouse、F/T 传感器驱动实现在内部 `rcore/peripheral` 模块）。

桥接示例还需 [`smrcore_sdk`](https://github.com/smore-robotics/smrcore_sdk) 与
可连通的机器人控制器。

## 文档

| 文档 | 说明 |
|---|---|
| [C++ 示例](examples/cpp/README.md) | 构建、仅读外设与 SDK 桥接可执行文件 |
| [Python 示例](examples/python/README.md) | wheel 安装与脚本用法 |
| [平台配置](docs/platform_setup.md) | SpaceMouse 设备节点、串口权限、机器人连接 |

## 快速开始

```bash
git clone https://github.com/smore-robotics/smrcore_peripherals.git
cd smrcore_peripherals

./scripts/download.sh    # 按 .peripherals-version / .sdk-version 下载 Release 制品
./scripts/build.sh       # 构建 C++ 示例（见 examples/cpp/README.md）

./build/bin/probe
```

Python 示例需单独安装 wheel，详见 [examples/python/README.md](examples/python/README.md)。

## 仓库结构

| 路径 | 内容 |
|---|---|
| `examples/cpp/` | C++ 示例（CMake 构建） |
| `examples/python/` | Python 只读脚本与 SDK 桥接骨架 |
| `docs/` | 平台配置说明 |
| `scripts/download.sh` | 下载 Release 中的 SDK / 外设制品 |
| `scripts/build.sh` | 配置并构建 C++ 示例 |

## 版本文件

| 文件 | 含义 |
|---|---|
| `.peripherals-version` | 期望的 `smrcore_peripherals` / `rcore-peripherals-py` Release 版本 |
| `.sdk-version` | 桥接示例期望的 `smrcore_sdk` / `rcore-sdk-py` Release 版本 |

请与 `scripts/download.sh` 实际消费的 GitHub Release 版本保持一致。

## Release 制品

制品发布在 **[Releases 页面](https://github.com/smore-robotics/smrcore_peripherals/releases)**：

| 制品 | 说明 |
|---|---|
| `rcore_peripherals-cpp-linux-x86_64-v<version>.tar.gz` | Linux x86_64 的 C++ 外设库 |
| `rcore_peripherals-cpp-windows-x86_64-v<version>.tar.gz` | Windows x86_64 的 C++ 外设库 |
| `rcore_peripherals_py-<version>-<python-tags>.whl` | Python wheel（按 Python ABI / 平台） |

桥接工作流另需从 [smrcore_sdk Releases](https://github.com/smore-robotics/smrcore_sdk/releases)
获取 `smrcore_sdk-cpp-*` 与 `rcore_sdk_py-*` 制品。

## 安全提示

> 机器人是危险设备。运行任何桥接或运动相关示例前，请确认工作空间已清空、急停可触达，
> 且外设输入不会导致非预期运动。F/T 与遥操作数据将直接影响控制器行为。

## 许可证

本仓库（示例、脚本与文档）以 [Apache License 2.0](LICENSE) 发布。预编译外设库
制品中打包的第三方组件，其许可证与归属声明随每个 release 压缩包一同提供。

<div align="center">

Copyright © Smartmore Corporation

</div>
