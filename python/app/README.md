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

## Pico 只读位置

安装 `rcore-peripherals-py` 后，即可读取 Pico 的位置和按键，与 SpaceMouse、六维力传感器使用同一个 wheel。

```bash
python3 python/app/app_peripherals_read_pico.py --port 8765
```

USB 连接头显后，在另一个终端配置端口转发：

```bash
adb reverse tcp:8765 tcp:8765
```

在头显浏览器打开 http://localhost:8765/ 并进入 VR。位置为 WebXR 参考空间 XYZ（米），未追踪显示 N/A；Ctrl+C 退出。

- [中凌主臂读取与单臂 i3 ServoJ 遥操](../../docs/site/zhonglin.md)：`app_peripherals_read_zhonglin.py` / `app_peripherals_zhonglin_i3_bridge.py`.

## Python 启动 Pico 双臂遥操

Python 入口复用 C++ SDK bridge；B3 支架变换、Grip 离合、J6 摇杆、Trigger 夹爪、奇异位形减速和断流停止与 C++ 程序一致。无需安装外设 wheel 或机器人 Python SDK。

先用配套的双臂 C++ SDK 构建：

```bash
./scripts/build.sh --with-sdk ON --with-pico ON --sdk-root /path/to/compatible-sdk
python3 python/app/app_peripherals_pico_bridge.py --help
```

确认机械臂工作空间和急停可用后启动：

```bash
python3 python/app/app_peripherals_pico_bridge.py \
  --robot-ip 192.168.3.232 --port 8765 --enable-motion \
  --gripper-force 80 --gripper-speed 100
```

使用已安装的程序时增加 `--bridge-executable /path/to/app_peripherals_pico_bridge`。
默认查找仓库 `build_Release/bin`、`build_Release/install/bin`，然后查找 PATH。
其他参数原样交给 C++ bridge 校验；不会自动添加 `--enable-motion`。
Python 进程直接替换为 C++ bridge，Ctrl+C 和退出码沿用原程序，不额外留下后台遥操进程。
不要同时运行 Pico 只读程序占用同一端口。Pico 网络连接步骤见 Pico 指南。
