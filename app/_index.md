> DIR: app/

# Overview

可执行示例与 SDK 桥接/标定工具。由 `CMakeLists.txt` 中 `add_peripheral_app()` 构建并 install 到 `${CMAKE_INSTALL_BINDIR}`。`app_peripherals_bridge` 与 `app_peripherals_ft_sensor_calib` 仅在 `SMR_PERIPHERAL_WITH_SDK=ON` 时编译。构建与运行说明见 `README.md` / `README.en.md`。

# Contents

- app_peripherals_probe.cpp: 打印 SpaceMouse/F/T 管理面状态（`GetInfo()`，不 `Start()`）
- app_peripherals_read_spacemouse.cpp: 定频循环打印归一化 `SpaceMouseSample`
- app_peripherals_read_spacemouse_raw.cpp: 定频循环打印 `SpaceMouseRawSample`（原始轴+按键）
- app_peripherals_read_ft_sensor.cpp: `FtSensorOptions` 初始化 → `WaitForFirstSample()` → 打印六维力/力矩
  - CLI: `--serial-port` `--sensor-type` `--baud-rate` `--sample-rate` `--stale-timeout-ms` `--read-buffer-size`
- app_peripherals_bridge.cpp: 单一 `Robot` SDK 会话内按 CLI 启动 SpaceMouse 和/或 F/T reader；默认两路都启用于 FDCC，`--spacemouse` / `--ft-sensor` 可显式选择单路或两路；连接成功后打印“正在发送外设消息”状态；需 `./scripts/download.sh` 后以 `--with-sdk ON` 构建
- app_peripherals_ft_sensor_calib.cpp: MoveJ 静态六维力标定（bias/重力/质心/yaw；SDK `GetFtSensorRawState` + Preview/Save）；仅 `--with-sdk ON` 时编译
