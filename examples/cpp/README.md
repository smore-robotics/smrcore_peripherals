# C++ 示例

[English](README.en.md) · **简体中文**

本目录包含基于 `smrcore_peripherals` C++ 包的示例，由仓库根目录的 CMake 构建。
桥接示例还需 [`smrcore_sdk`](https://github.com/smore-robotics/smrcore_sdk) 与可连通的机器人控制器。

平台权限与设备节点配置见 [平台配置](../../docs/platform_setup.md)。

## 构建

在仓库根目录执行：

```bash
./scripts/download.sh    # 按 .peripherals-version / .sdk-version 下载 Release 制品
./scripts/build.sh       # 配置并构建 C++ 示例
```

可执行文件位于 `build/bin/`。`scripts/download.sh` 将制品拉取到 `third_party/`。

若仅有外设 C++ 包、不需要 SDK 桥接示例：

```bash
cmake -S . -B build -DSMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=OFF \
  -DCMAKE_PREFIX_PATH=third_party/smrcore_peripherals
cmake --build build
```

或使用：

```bash
SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=OFF ./scripts/build.sh
```

## 仅读外设

以下目标仅链接 `smrcore_peripherals` C++ 包：

| 可执行文件 | 说明 |
|---|---|
| `probe` | 探测 SpaceMouse 与 F/T 传感器型号、采样率与连接状态 |
| `read_spacemouse` | 解码后的 SpaceMouse 6-DOF + 握把采样（`--device`、`--sample-rate`） |
| `read_spacemouse_raw` | 原始 HID 轴/按键采样 |
| `read_ft_sensor` | 串口六维力矩流，含 `WaitForFirstSample` 与完整串口 CLI |

```bash
./build/bin/probe
./build/bin/probe --serial-port /dev/ttyUSB0

./build/bin/read_spacemouse --device /dev/input/event5 --sample-rate 125
./build/bin/read_spacemouse_raw
./build/bin/read_ft_sensor --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

## 机器人 SDK 桥接

默认在 `SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=ON` 时构建。需要 `smrcore_sdk`、
可连通的机器人，以及本机已连接的外设。

| 可执行文件 | 说明 |
|---|---|
| `peripherals_sdk` | 通过 `robot.Peripheral().Update*Sample()` 推送 SpaceMouse 和/或外部 F/T 数据（默认两路均开，供 FDCC 使用） |

```bash
# FDCC：SpaceMouse + F/T（先在 .cpp 顶部修改设备常量）
./build/bin/peripherals_sdk
./build/bin/peripherals_sdk --robot <robot-ip>

# 仅遥操作或仅力传感器
./build/bin/peripherals_sdk --spacemouse
./build/bin/peripherals_sdk --ft-sensor
```

`--robot <robot-ip>` 可省略；省略时向 SDK 传入空 IP，由 SDK 使用默认本机连接行为。
桥接进程应在启用遥操作或 FDCC **之前**启动，以便 rcore 收到最新的外部输入。

## 安全提示

> 机器人是危险设备。运行桥接示例前，请确认工作空间已清空、急停可触达，
> 且外设输入不会导致非预期运动。F/T 与遥操作数据将直接影响控制器行为。
