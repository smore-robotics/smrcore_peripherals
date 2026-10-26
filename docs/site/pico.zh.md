# Pico 读取与双臂遥操

## 两条链路

| 用途 | 程序 | 机器人 SDK |
| --- | --- | --- |
| 查看头显、双手柄位置和按键 | C++ `app_peripherals_read_pico` 或 Python `app_peripherals_read_pico.py` | 不需要 |
| B3 双臂遥操及 DH 夹爪 | C++ `app_peripherals_pico_bridge` | 需要兼容双臂 SDK |

SpaceMouse bridge 上传采样给柜端消费；Pico bridge 在应用层将手柄位姿映射为各臂目标，再调用 SDK。两者都保持读取层与控制层分离，但不是同一个柜端输入接口。

## 1. 安装与只读验证

```bash
./scripts/build.sh --with-sdk OFF --tests ON
./build_Release/bin/app_peripherals_read_pico --port 8765
```

或者安装 Python wheel 后运行：

```bash
python python/app/app_peripherals_read_pico.py --port 8765
```

两个读取器二选一，不能同时占用同一个端口。USB 连接并授权 adb 后，在另一终端执行：

```bash
adb devices
adb reverse tcp:8765 tcp:8765
```

在头显浏览器打开 `http://localhost:8765/`，进入 VR 并确认手柄正在追踪。

输出包含 `seq`、`ref`、头显及左右手柄 `xyz[m]`、`grip` 和 `trigger`。位置属于 WebXR 参考空间，单位米、保留四位小数，**不是机械臂 Base/TCP 坐标**。未追踪时为 `N/A`。没有新样本时 `GetSample()` 返回空，不应当作新的有效位置。

普通 Wi-Fi 中 localhost 指头显自身；需使用头显能访问且受信任的 HTTPS 入口，同时转发 WebSocket。不能直接把上述地址改成电脑 HTTP IP 就假定 WebXR 可用。上述命令使用 adb 转发。

## 2. 编译双臂程序

```bash
./scripts/build.sh --with-sdk ON --with-pico ON \
  --sdk-root /path/to/compatible-sdk
./build_Release/bin/app_peripherals_pico_bridge --help
```

SDK 需要 `sdk/dual_arm_robot.hpp` 及匹配动态库。发布使用的 SDK 0.2.0 包不包含该头文件，因此预编译包不包含 Pico 遥操程序。请使用提供此接口及匹配运行库的 SDK 从源码构建。

示例**内置 B3 支架变换**，只适用于对应安装构型，没有通用支架配置参数。连接使用 `DualArmRobot::Initialize(robot_ip)`，当前 CLI 没有 domain/interface 覆盖参数。

## 3. 操作方式与启动

下述命令会进入实机控制流程，不是只读命令。先完成只读验证，确认两臂、安装构型、工具和工作区适配，并保持急停可触达。

```bash
./build_Release/bin/app_peripherals_pico_bridge \
  --enable-motion --robot-ip <robot-ip> --port 8765
```

先停止只读程序，避免端口冲突，再启动 bridge 并连接头显。

| 输入 | 行为 |
| --- | --- |
| 按住对应手柄 Grip | 离合后按手柄增量控制对应机械臂 |
| 松开 Grip | 取消旧目标，执行规划的 ServoJ 停止流程 |
| Grip 按住时摇杆 Y | ServoJ 控制对应臂 J6；回中后从实测 TCP 重新接入 ServoP |
| Trigger 按下沿 | 切换对应 DH 夹爪开/闭；按下阈值 0.75，释放阈值 0.25 |
| Ctrl+C | 请求退出并执行停止清理 |

**启动会初始化两侧 DH 夹爪、写入力度/速度并请求打开。** 夹爪命令成功仅说明写入被接受，当前不回读实际位置或夹持状态。

## 4. CLI 参数

| 参数 | 默认值 / 范围 | 作用 |
| --- | --- | --- |
| `--enable-motion` | 必须显式指定 | 确认进入实机控制 |
| `--robot-ip` | 目标柜 IP | 双臂 SDK 连接地址 |
| `--port` | 8765；1–65535 | WebXR 服务端口 |
| `--translation-only` | 默认关闭 | 仅平移，不映射旋转 |
| `--room-fixed` | 默认关闭 | 使用房间固定方向，替代头部 yaw 离合方向 |
| `--operator-frame` | same-side / face-to-face；默认 same-side | 操作者朝向映射 |
| `--translation-scale` | 0.8；0.1–3.0 | 平移映射比例 |
| `--rotation-scale` | 1.0；0.1–3.0 | 旋转映射比例 |
| `--motion-scale` | 不指定时保留上述默认值 | 同时设置平移与旋转比例，0.1–3.0 |
| `--j6-speed` | 0.8 rad/s；0.1–3.0 | 摇杆 J6 请求速度，仍受柜端限速约束 |
| `--gripper-force` | 80；20–100 | DH 夹爪力度参数，不是牛顿 |
| `--gripper-speed` | 100；1–100 | DH 夹爪速度参数 |

参数按命令行顺序解析；`--motion-scale` 会覆盖此前的两项比例，后续单项参数可再次覆盖。

## 5. 暂停、恢复及限制

接近奇异区时，条件数 30–60 使用二次减速；离开时使用加倍门槛。短暂 Jacobian 超时会冻结并制动受影响的单臂；Pico 输入超过 250 ms 未更新时会冻结并制动双臂，同时取消未发送夹爪目标，输入恢复后从实测 TCP 重新离合。机器人状态过期、控制器故障和检查基础设施故障仍会停止会话。

候选目标检查尚未完成或不通过时保持最后获批目标，不能理解为强行越过限位。J6 不使用笛卡尔奇异点减速比例，但仍受关节控制约束。

Python 启动入口复用同一 C++ bridge。运动示例需在目标构型上完成现场验证。

## Python 启动 Pico 双臂遥操

Python 入口复用 C++ SDK bridge；B3 支架变换、Grip 离合、J6 摇杆、Trigger 夹爪、奇异位形减速和断流停止与 C++ 程序一致。无需安装外设 wheel 或机器人 Python SDK。

先用配套的双臂 C++ SDK 构建：

```bash
./scripts/build.sh --with-sdk ON --with-pico ON --sdk-root /path/to/compatible-sdk
python3 python/app/app_peripherals_pico_bridge.py --help
```

确认机械臂工作空间和急停可用后启动：

```bash
python3 python/app/app_peripherals_pico_bridge.py \
  --robot-ip 192.168.3.232 --port 8765 --enable-motion \
  --gripper-force 80 --gripper-speed 100
```

使用已安装的程序时增加 `--bridge-executable /path/to/app_peripherals_pico_bridge`。
默认查找仓库 `build_Release/bin`、`build_Release/install/bin`，然后查找 PATH。
其他参数原样交给 C++ bridge 校验；不会自动添加 `--enable-motion`。
Python 进程直接替换为 C++ bridge，Ctrl+C 和退出码沿用原程序，不额外留下后台遥操进程。
不要同时运行 Pico 只读程序占用同一端口。Pico 网络连接步骤见 Pico 指南。
