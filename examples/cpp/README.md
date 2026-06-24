# C++ 示例

[English](README.en.md) · **简体中文**

本目录保留对外示例源码；当前源码仓库推荐优先使用仓库根目录 `app/` 下的 `app_peripherals_*` 工具。两者使用同一套 `smrcore::peripherals` C++ API。

平台权限与设备节点配置见 [平台配置](../../docs/platform_setup.md)。

## 构建推荐路径

在仓库根目录执行：

```bash
# 不需要机器人 SDK，只构建外设库、读数 app 和测试
./scripts/build.sh --with-sdk OFF --tests ON

# 需要 app_peripherals_bridge 时
./scripts/download.sh
./scripts/build.sh --with-sdk ON --tests ON
```

当前推荐可执行文件位于 `build_Release/install/bin/`：

| 可执行文件 | 说明 |
|---|---|
| `app_peripherals_probe` | 探测 SpaceMouse 与 F/T 传感器型号、采样率与连接状态 |
| `app_peripherals_read_spacemouse` | 解码后的 SpaceMouse 6-DOF + 夹爪目标采样 |
| `app_peripherals_read_spacemouse_raw` | 原始 Linux input 轴/按键采样 |
| `app_peripherals_read_ft_sensor` | 串口六维力矩流，含 `WaitForFirstSample` 与完整串口 CLI |
| `app_peripherals_bridge` | 通过 `robot.Peripheral().Update*Sample()` 推送 SpaceMouse 和/或 F/T 数据 |

## 仅读外设

```bash
./build_Release/install/bin/app_peripherals_probe
./build_Release/install/bin/app_peripherals_probe --serial-port /dev/ttyUSB0

./build_Release/install/bin/app_peripherals_read_spacemouse --device /dev/input/event5 --sample-rate 125
./build_Release/install/bin/app_peripherals_read_spacemouse_raw --device /dev/input/event5
./build_Release/install/bin/app_peripherals_read_ft_sensor \
  --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

## 机器人 SDK 桥接

`app_peripherals_bridge` 需要 `smrcore_sdk`、可连通的机器人，以及本机已连接的外设。

```bash
# 默认同时启用 SpaceMouse 和 F/T，适合 FDCC 双外设输入
./build_Release/install/bin/app_peripherals_bridge --robot <robot-ip>

# 仅遥操作或仅力传感器
./build_Release/install/bin/app_peripherals_bridge --robot <robot-ip> --spacemouse
./build_Release/install/bin/app_peripherals_bridge --robot <robot-ip> --ft-sensor

# 覆盖设备参数
./build_Release/install/bin/app_peripherals_bridge --robot <robot-ip> \
  --spacemouse-device /dev/input/event5 \
  --ft-serial-port /dev/ttyUSB0 \
  --ft-sensor-type xjc_serial
```

`--robot <robot-ip>` 可省略；省略时向 SDK 传入空 IP，由 SDK 使用默认连接行为。bridge 应在启用遥操作或 FDCC 之前启动，以便 rcore 收到新鲜 external input。

## 安全提示

> 机器人是危险设备。运行 bridge 示例前，请确认工作空间已清空、急停可触达，且外设输入不会导致非预期运动。F/T 与遥操作数据将直接影响控制器行为。
