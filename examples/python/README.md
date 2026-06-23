# Python 示例

[English](README.en.md) · **简体中文**

从 [GitHub Releases](https://github.com/smore-robotics/smrcore_peripherals/releases)
下载 wheel 并安装（版本应与 `.peripherals-version` 一致；桥接工作流还需与
`.sdk-version` 对齐的机器人 SDK wheel）：

```bash
VERSION=0.0.1  # 与 ../../.peripherals-version 一致
PY_TAG=cp310-cp310-linux_x86_64   # Windows: cp310-cp310-win_amd64
curl -L -O "https://github.com/smore-robotics/smrcore_peripherals/releases/download/v${VERSION}/rcore_peripherals_py-${VERSION}-${PY_TAG}.whl"
python3 -m pip install "./rcore_peripherals_py-${VERSION}-${PY_TAG}.whl"
```

在本目录（`examples/python/`）下运行脚本。

PyPI 包名：`rcore-peripherals-py`；import 名：`rcore_peripherals`。

设备权限与节点配置见 [平台配置](../../docs/platform_setup.md)。

## 仅读外设

| 脚本 | 说明 |
|--------|---------|
| `probe.py` | 打印 SpaceMouse 与 F/T 传感器型号、采样率与连接状态 |
| `read_spacemouse.py` | 流式输出解码后的 6-DOF + 握把采样 |
| `read_spacemouse_raw.py` | 流式输出原始 HID 轴/按键采样 |
| `read_ft_sensor.py` | 在首帧有效数据后流式输出六维力矩 |

### 用法

```bash
python3 probe.py
python3 probe.py --serial-port /dev/ttyUSB1

python3 read_spacemouse.py
python3 read_spacemouse.py --device /dev/input/event5 --sample-rate 125

python3 read_spacemouse_raw.py --device /dev/input/event5

python3 read_ft_sensor.py --serial-port /dev/ttyUSB0 --sensor-type xjc_serial \
  --baud-rate 460800 --sample-rate 1000
```

## 机器人 SDK 桥接

| 脚本 | 对应 C++ 示例 |
|--------|-----------------|
| `peripherals_sdk.py` | `peripherals_sdk` |

桥接脚本需同时安装外设 wheel 与机器人 SDK wheel（版本见仓库根目录）：

```bash
VERSION=0.0.1       # 与 ../../.peripherals-version 一致
SDK_VERSION=0.0.3   # 与 ../../.sdk-version 一致
PY_TAG=cp310-cp310-linux_x86_64   # Windows: cp310-cp310-win_amd64

curl -L -O "https://github.com/smore-robotics/smrcore_peripherals/releases/download/v${VERSION}/rcore_peripherals_py-${VERSION}-${PY_TAG}.whl"
python3 -m pip install "./rcore_peripherals_py-${VERSION}-${PY_TAG}.whl"

curl -L -O "https://github.com/smore-robotics/smrcore_sdk/releases/download/v${SDK_VERSION}/rcore_sdk_py-${SDK_VERSION}-${PY_TAG}.whl"
python3 -m pip install "./rcore_sdk_py-${SDK_VERSION}-${PY_TAG}.whl"
```

### 用法

```bash
# FDCC：SpaceMouse + F/T（先在脚本顶部修改设备常量）
python3 peripherals_sdk.py
python3 peripherals_sdk.py --robot 192.168.1.100

# 仅遥操作或仅力传感器
python3 peripherals_sdk.py --spacemouse
python3 peripherals_sdk.py --ft-sensor
python3 peripherals_sdk.py --spacemouse --ft-sensor
```

`--robot` 可省略；省略时 SDK 使用默认本机连接行为。设备路径与串口参数为
`peripherals_sdk.py` 顶部常量（与 C++ 示例一致）。若需 CLI 驱动设备选项，请使用
`rcore/peripheral/python/app/app_peripherals_bridge.py`。

## 安全提示

> 机器人是危险设备。运行桥接脚本前，请确认工作空间已清空、急停可触达，
> 且外设输入不会导致非预期运动。
