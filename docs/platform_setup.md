# 平台配置

[English](platform_setup.en.md) · **简体中文**

## SpaceMouse

在 Linux 上，运行示例的用户必须能读取所选 `/dev/input/event*` 设备节点。
可通过 udev 规则配置，或将用户加入对该设备有读权限的组。

可选参数 `--device` 指定 event 节点；省略时使用默认发现路径。
`--sample-rate` 设置读数轮询频率（Hz）；`read_spacemouse` 解码读数默认 125 Hz。

## F/T 传感器

串口六维力传感器需要用户对串口设备具备读写权限，例如 `/dev/ttyUSB0`。
示例默认传感器类型为 `xjc_serial`、波特率 `460800`，可通过 CLI 覆盖。

`probe` 与 `read_ft_sensor` 支持 `--serial-port`。桥接与读数示例会调用
`WaitForFirstSample()`；若首帧有效数据超时则退出。

## 机器人连接

桥接示例 `peripherals_sdk` 需要可连通的机器人控制器，且 `smrcore_sdk` 制品版本需匹配。
在启用遥操作或 FDCC **之前**启动桥接进程，以便 rcore 收到最新的外部输入。
