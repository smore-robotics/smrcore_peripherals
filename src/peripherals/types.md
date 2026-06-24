> SRC: types.hpp
> DIR: src/peripherals/

# types.hpp

外设公共 DTO，字段语义与机器人 SDK 外设采样类型对齐。

## 类型

- `GripperCommand`: Open/Close 夹爪锁存目标
- `PeripheralOptions`: 通用 Initialize 选项（采样率）
- `FtSensorOptions`: 力传感器 Initialize 选项（串口、协议类型、波特率、首帧等待参数等）
- `SpaceMouseOptions`: SpaceMouse Initialize 选项（设备路径、轴映射、死区等）
- `PeripheralInfo`: 低频设备/诊断信息
- `SpaceMouseSample`: 归一化六轴 + gripper + timestamp_sec
- `SpaceMouseRawSample`: 原始 axes[6] + buttons + timestamp_sec
- `FtSensorSample`: fx/fy/fz/tx/ty/tz + timestamp_sec

## 流输出

- `operator<<(ostream, SpaceMouseSample)`: `x y z roll pitch yaw gripper timestamp`
- `operator<<(ostream, SpaceMouseRawSample)`: `buttons=... axes=...`
- `operator<<(ostream, FtSensorSample)`: `fx fy fz tx ty tz`
