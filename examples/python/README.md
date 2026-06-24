# Python 示例

[English](README.en.md) · **简体中文**

当前源码仓库可直接构建 `rcore_peripherals` Python wheel。示例脚本位于本目录与 `python/app/`，两者使用同一套 Python API。

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
| `probe.py` | 打印 SpaceMouse 与 F/T 传感器型号、采样率与连接状态 |
| `read_spacemouse.py` | 流式输出解码后的 6-DOF + 夹爪采样 |
| `read_spacemouse_raw.py` | 流式输出原始轴/按键采样 |
| `read_ft_sensor.py` | 在首帧有效数据后流式输出六维力矩 |

```bash
python3 examples/python/probe.py --serial-port /dev/ttyUSB0
python3 examples/python/read_spacemouse.py --device /dev/input/event5 --sample-rate 125
python3 examples/python/read_spacemouse_raw.py --device /dev/input/event5
python3 examples/python/read_ft_sensor.py \
  --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

`python/app/` 下还有与安装产物一起维护的脚本：

```bash
python3 python/app/app_peripherals_read_spacemouse.py
python3 python/app/app_peripherals_read_ft_sensor.py --serial-port /dev/ttyUSB0
```

## 机器人 SDK 桥接

Python bridge 脚本 `python/app/app_peripherals_bridge.py` 需要同时安装外设 wheel 和机器人 SDK Python 包。若当前 Python SDK 尚未暴露与 C++ 一致的 `Peripheral().Update*Sample()` API，请使用 C++ `app_peripherals_bridge` 作为生产 bridge。

## 安全提示

> 机器人是危险设备。运行 bridge 脚本前，请确认工作空间已清空、急停可触达，且外设输入不会导致非预期运动。
