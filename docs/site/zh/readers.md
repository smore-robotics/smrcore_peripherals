# 外设读取

以下命令在仓库根目录执行，仅采集外设输入。

## SpaceMouse

先探测输入设备，再将示例节点替换为实际节点：

```bash
./build_Release/bin/app_peripherals_probe
python python/app/app_peripherals_read_spacemouse.py --device /dev/input/event5 --sample-rate 125
```

设备权限参见[平台配置](https://github.com/smore-robotics/smrcore_peripherals/blob/main/docs/platform_setup.md)。

## 六维力/力矩传感器

```bash
python python/app/app_peripherals_read_ft_sensor.py \
  --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

串口节点、协议及波特率需与实际传感器一致。

## Pico 位置与按键

```bash
python python/app/app_peripherals_read_pico.py --port 8765
```

USB 调试链路：安装 adb、连接并授权头显后，在另一个终端执行：

```bash
adb devices
adb reverse tcp:8765 tcp:8765
```

在头显浏览器打开 `http://localhost:8765/` 并进入 VR。位置是 WebXR 参考空间 XYZ，单位米；未追踪显示 N/A，Ctrl+C 退出。

!!! note "Wi-Fi 与 localhost"
    上面的 localhost 用法依赖 adb 端口反向转发。普通 Wi-Fi 连接中的 localhost 指头显自己；直接访问电脑 HTTP 地址也可能不满足 WebXR 安全上下文要求。Wi-Fi 方案需要头显可访问的 HTTPS 服务与受信任证书，不能只把 localhost 换成电脑 IP。

读取成功不等于已连接机器人。双臂接入见[机器人与 Pico 接入](robot-integration.md)。
