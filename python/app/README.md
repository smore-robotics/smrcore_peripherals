# Python 应用

[English](README.en.md) · **简体中文**

本目录包含与 C++ `app/` 对应的 Python 脚本，使用 `rcore_peripherals` Python API。

设备权限与节点配置见 [平台配置](../../docs/platform_setup.md)。

## 构建和安装 wheel

在仓库根目录执行：

```bash
./scripts/build.sh --with-sdk OFF --tests OFF
./scripts/build_py.sh
python3 -m pip install --force-reinstall python/dist/*.whl
```

包名：`rcore-peripherals-py`；import 名：`rcore_peripherals`。

## 仅读外设

| 脚本 | 说明 |
|---|---|
| `app_peripherals_read_spacemouse.py` | 流式输出解码后的 6-DOF + 夹爪采样 |
| `app_peripherals_read_ft_sensor.py` | 在首帧有效数据后流式输出六维力矩 |
| `app_peripherals_bridge.py` | 向机器人 SDK 推送 SpaceMouse / F/T 采样 |
| `app_peripherals_ft_sensor_calib.py` | MoveJ 静态六维力标定（Preview / `--save`） |

```bash
python3 python/app/app_peripherals_read_spacemouse.py --device /dev/input/event5 --sample-rate 125
python3 python/app/app_peripherals_read_ft_sensor.py \
  --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

探测与原始 SpaceMouse 采样请使用 C++ `app_peripherals_probe` 与 `app_peripherals_read_spacemouse_raw`，见 [app/README.md](../../app/README.md)。

## 机器人 SDK 桥接

`app_peripherals_bridge.py` 需要同时安装外设 wheel 和机器人 SDK Python 包（`rcore-sdk-py`）。若当前 Python SDK 尚未暴露与 C++ 一致的 `Peripheral().Update*Sample()` API，请使用 C++ `app_peripherals_bridge` 作为生产 bridge。

```bash
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip>
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip> --spacemouse
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip> --ft-sensor
```

机器人 SDK 安装说明见 [python/README.md](../README.md)。

## 六维力静态标定

`app_peripherals_ft_sensor_calib.py` 镜像 C++ `app_peripherals_ft_sensor_calib`，仅依赖 `rcore-sdk-py`（不直接打开串口）。须先由 bridge 注入 raw。

```bash
# 1) 注入 raw（Python 或 C++ bridge 均可）
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip> --ft-sensor

# 2) Preview；确认合格后再 --save
python3 python/app/app_peripherals_ft_sensor_calib.py --robot-ip <robot-ip>
python3 python/app/app_peripherals_ft_sensor_calib.py --robot-ip <robot-ip> --save
```

CLI 与 C++ 对齐：`--robot-ip`、`--nominal`、`--settle-ms`、`--sample-ms`、`--velocity-ratio`、`--joint0-deg`、`--joint5-deg`、`--save`。

## 安全提示

> 机器人是危险设备。运行 bridge 或标定脚本前，请确认工作空间已清空、急停可触达，且外设输入不会导致非预期运动。力控前须先完成一次 `--save` 标定。
