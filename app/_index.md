> DIR: app/

# Overview

可执行示例与 SDK 桥接工具。由 `CMakeLists.txt` 中 `add_peripheral_app()` 构建并 install 到 `${CMAKE_INSTALL_BINDIR}`。构建与运行说明见 `README.md` / `README.en.md`。

# Contents

- app_peripherals_probe.cpp: 打印 SpaceMouse/F/T 管理面状态（`GetInfo()`，不 `Start()`）
- app_peripherals_read_spacemouse.cpp: 定频循环打印归一化 `SpaceMouseSample`
- app_peripherals_read_spacemouse_raw.cpp: 定频循环打印 `SpaceMouseRawSample`（原始轴+按键）
- app_peripherals_read_ft_sensor.cpp: `FtSensorOptions` 初始化 → `WaitForFirstSample()` → 打印六维力/力矩
  - CLI: `--serial-port` `--sensor-type` `--baud-rate` `--sample-rate` `--stale-timeout-ms` `--read-buffer-size`
- app_peripherals_bridge.cpp: 单一 `Robot` SDK 会话内按 CLI 启动 SpaceMouse 和/或 F/T reader；默认两路都启用于 FDCC，`--spacemouse` / `--ft-sensor` 可显式选择单路或两路；需 `./scripts/download.sh` 后以 `--with-sdk ON` 构建

本仓库以 `--with-sdk OFF` 构建时 bridge 应用仅打印提示。
