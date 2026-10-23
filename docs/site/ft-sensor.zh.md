# 六维力传感器

设备输入 → 外设读取 → 机器人 SDK Peripheral 接口 → 柜端控制。

## 读取输入

```bash
python3 python/app/app_peripherals_read_ft_sensor.py --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

## 机器人接入

```bash
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip> --ft-sensor
```

安装外设 wheel 和机器人 Python SDK。bridge 上传外设样本，实际运动由柜端控制模式及配置决定。C++ 入口与 SDK 构建步骤见[机器人接入](robot-integration.md)。

## 配置与适用范围

按实际硬件选择设备节点、协议与波特率，见[平台配置](platform.md)。

静态标定流程见[Python 示例](python.md)。
