# Pico 接入与验证状态

## 当前链路

- 基础 C++ 外设库和 Python wheel 只读取 Pico 数据，不包含机器人 SDK 或遥操核心。
- Python 读取示例：`python/app/app_peripherals_read_pico.py`，构建沿用 `scripts/build.sh` 和 `scripts/build_py.sh`。
- 双臂遥操使用独立 C++ `app_peripherals_pico_bridge`，通过 `--with-sdk ON --with-pico ON` 显式构建；依赖匹配的双臂 C++ SDK。
- SpaceMouse bridge 上传采样给柜端；Pico bridge 在应用层映射位姿并调用 ServoP。两者保持外设读取与机器人控制分离。
- 独立 Python 双臂控制示例尚未提供。

## 适用范围

本示例内置 B3 支架变换，适用于对应 B3 双臂安装构型，不是通用双臂配置。其他支架或安装方式需重新配置并验证坐标变换。

核心在 `teleop/pico_teleop.cpp`，包含坐标转换、Grip 离合、目标检查、停止、J6 和末端板 DH 夹爪控制。夹爪通过各臂 `Robot.EndBoard()` 写入 DH 命令，目前不回读实际位置或夹持状态。

连接使用 `DualArmRobot::Initialize(robot_ip)`，没有 domain/interface 参数；编译通过不代表任意控制柜通信配置均可连接。

## SDK 发布包要求

官方 v0.2.0 C++ 包的库中已有双臂符号，但安装清单漏掉 `sdk/dual_arm_robot.hpp`。SDK 修复提交为 `0852bae`，正式发布前应使用补齐该头文件且与运行库匹配的安装包。当前 `.sdk-version` 为 `0.2.0`，不能据此认定默认下载包可构建双臂遥操。

## 已完成验证

- 官方 v0.2.0 库仅补入同版本双臂头文件后，独立双臂消费者及 Pico bridge 编译链接通过，`--help` 正常；未替换库。构建使用 `BUILD_SHARED_LIBS=ON`。
- 基础 Python wheel 构建、6 项 Python 测试通过。
- 5 项 CTest、2 项 Python 驱动的停止模拟测试通过。
- 未进行实机运动验收；松手停止、断流恢复和夹爪行为仍需实机验证。

使用说明见 [链路说明](../python/TELEOP.md)，实现索引见 [遥操核心](../teleop/_index.md)。
