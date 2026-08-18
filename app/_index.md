> DIR: app/

# Overview

可执行示例与 SDK 桥接/标定工具。由 `CMakeLists.txt` 中 `add_peripheral_app()` 构建并 install 到 `${CMAKE_INSTALL_BINDIR}`。`app_peripherals_bridge`、`app_peripherals_pico_bridge` 与 `app_peripherals_ft_sensor_calib` 仅在 `SMR_PERIPHERAL_WITH_SDK=ON` 时编译。构建与运行说明见 `README.md` / `README.en.md`。

# Contents

- app_peripherals_probe.cpp: 打印 SpaceMouse/F/T 管理面状态（`GetInfo()`，不 `Start()`）
- app_peripherals_read_spacemouse.cpp: 定频循环打印归一化 `SpaceMouseSample`
- app_peripherals_read_spacemouse_raw.cpp: 定频循环打印 `SpaceMouseRawSample`（原始轴+按键）
- app_peripherals_read_ft_sensor.cpp: `FtSensorOptions` 初始化 → `WaitForFirstSample()` → 打印六维力/力矩
  - CLI: `--serial-port` `--sensor-type` `--baud-rate` `--sample-rate` `--stale-timeout-ms` `--read-buffer-size`
- app_peripherals_read_pico.cpp: 启动 `Pico` localhost WebXR 服务并打印头显/双手柄采样；仅有 `--port`，不连接机器人
- app_peripherals_bridge.cpp: 单一 `Robot` SDK 会话内按 CLI 启动 SpaceMouse 和/或 F/T reader；默认两路都启用于 FDCC，`--spacemouse` / `--ft-sensor` 可显式选择单路或两路；连接成功后打印“正在发送外设消息”状态；需 `./scripts/download.sh` 后以 `--with-sdk ON` 构建
- app_peripherals_ft_sensor_calib.cpp: MoveJ 静态六维力标定（bias/重力/质心/yaw；SDK `GetFtSensorRawState` + Preview/Save）；仅 `--with-sdk ON` 时编译
- app_peripherals_pico_bridge.cpp: 组合 `Pico` 与 `rcoresdk` 的双臂 ServoP/关节透传桥接；要求显式 `--enable-motion` 安全确认；启动时缓存最近新鲜样本判断双 Grip 连续松开 0.5 秒；将左右 Trigger 边沿映射夹爪开/关并通过 `UpdateSpaceMouseSample()` 发布；按住对应 Grip 时，左右手柄 thumbstick 的 Y 轴分别直接控制对应手臂本地 J6，使用 `JointPassthrough`、死区 `0.18` 和默认 `0.8 rad/s`（可用 `--j6-speed` 调整）；B3 GetBasePose≈I 且 reference 为 `base`/`left_b3_arm_base`/`right_b3_arm_base` 时用整机 URDF `*_base_joint` 后乘 `Rz(π)` 对齐 MJCF arm-base（与键盘双臂 World 一致；漏掉 `base` 会使 world≡arm）；1kHz 环不周期性打 GetBasePose RPC，仅 `reference_frame` 变化时重解析；WebXR/RobotState 短中断冻结后按住 Grip 从当前 TCP 重新离合，不强制松开；退出时左右臂分别 ServoJ 刹停到速度 <0.01 rad/s 再 Disable 并核对 `GetMotorStatus().enabled`（Cia402 运动中拒下使能且 RPC 无返回值，否则只有静止的那一侧会掉使能）；仅本程序包含 `--robot-ip`，且只在 `--with-sdk ON` 时构建

本仓库以 `--with-sdk OFF` 构建时不编译 bridge 与 F/T 标定应用。
