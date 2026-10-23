# C++ 示例

```bash
./scripts/build.sh --with-sdk OFF --tests ON
```

| 程序 | 用途 |
| --- | --- |
| app_peripherals_probe | 探测输入设备 |
| app_peripherals_read_spacemouse_raw | SpaceMouse 原始数据 |
| app_peripherals_read_pico | Pico 位置与按键 |
| app_peripherals_bridge | 可选机器人 SDK 采样桥接 |
| app_peripherals_pico_bridge | 可选 B3 双臂遥操 |

使用 `--help` 查看参数。[机器人接入](robot-integration.md)说明可选编译要求；[完整 C++ 应用说明](https://github.com/smore-robotics/smrcore_peripherals/blob/main/app/README.md)。
