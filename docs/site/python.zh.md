# Python 示例

基础 wheel 导入名为 `rcore_peripherals`，读取外设不依赖机器人 SDK。

| 脚本 | 用途 |
| --- | --- |
| app_peripherals_read_spacemouse.py | SpaceMouse 采样 |
| app_peripherals_read_ft_sensor.py | 六维力/力矩采样 |
| app_peripherals_read_pico.py | Pico 位置与按键 |
| app_peripherals_bridge.py | 采样桥接，需要兼容机器人 Python SDK |
| app_peripherals_ft_sensor_calib.py | 机器人六维力标定，需要机器人 SDK |

先看[安装](getting-started.md)和[外设读取](readers.md)。后两个脚本需另装机器人 SDK，若缺少 Peripheral 更新 API，应使用 C++ bridge。外设 wheel 不包含 Pico 机器人遥操核心。
