# smrcore_peripherals

公开的外设 SDK 源码仓库，负责 **SpaceMouse（3D 鼠标）** 与 **六维力/力矩传感器（F/T Sensor）** 的本地读取、协议解析、采样归一化，以及调试/桥接工具。

本仓库不依赖 rcore 源码，也不维护机器人 IDL。需要将外设数据送入机器人控制器时，应通过 rcore SDK 的 `Robot::Peripheral().UpdateSpaceMouseSample()` / `UpdateFtSensorSample()` 高频外部输入接口，而不是 RPC。

## 主要功能

| 模块 | 说明 |
|------|------|
| **SpaceMouse** | 通过 Linux input 子系统读取原始事件，输出归一化六自由度 + 夹爪目标状态 |
| **F/T Sensor** | 支持坤维（`kunwei_serial`）与鑫精诚 XJC（`xjc_serial`）串口协议，输出六维力/力矩 |
| **C++ SDK** | `smrcore::peripherals` 命名空间，统一外设生命周期：`Initialize` → `Start` → `GetSample` → `Stop` → `Shutdown` |
| **Python 绑定** | 基于 pybind11 的 `rcore_peripherals` 包，API 与 C++ 对齐 |
| **示例应用** | `app/` 下提供探测、读数、SDK 桥接与标定辅助工具 |

## 架构概览

```
peripherals/          公开门面 API（FtSensor、SpaceMouse、types.hpp）
    ↓
protocols/            协议与设备 IO（串口、Linux input、帧解析）
    ↓
common/               时钟、consume-on-read 槽、资源路径解析等内部工具
```

- **门面层**（`src/peripherals/`）：派生自 `Peripheral` 基类，管理后台线程与采样槽，对外暴露 `*Options` 配置结构体。
- **协议层**（`src/protocols/`）：品牌无关的 `SerialReader` + 各品牌 `Parser`/`Reader`；SpaceMouse 使用 `linux_input_reader`。
- 类型语义与 rcore SDK `data.hpp` 中的外设采样类型对齐，便于桥接。

## 目录结构

```
peripheral/
├── app/              # 可执行示例与桥接工具
├── cmake/            # CMake 包配置模板（find_package）
├── infra/conan/      # Conan profile
├── python/           # Python 绑定、示例脚本与测试
├── scripts/          # 构建与测试脚本
├── src/
│   ├── common/       # 内部工具（时钟、LatestSlot、资源路径）
│   ├── peripherals/  # 公开 API（types、Peripheral、FtSensor、SpaceMouse）
│   └── protocols/    # 串口/Linux input 协议实现
└── unittests/        # 无硬件单元测试
```

公开头文件位于 `src/peripherals/`，安装后映射到 `<peripherals/...>`。聚合入口：

```cpp
#include "peripherals/peripherals.hpp"
```

## 配置方式

外设参数通过 **C++ 配置结构体**（或 Python 同名类）在 `Initialize()` 时传入。运行中不支持原地修改，需 `Stop()` → `Shutdown()` 后以新参数重新 `Initialize()`。

### 公共基类

**`PeripheralOptions`**（`src/peripherals/types.hpp`）：

| 字段 | 含义 | 默认值 |
|------|------|--------|
| `sample_rate_hz` | 请求采样/输出频率 [Hz]；≤0 时使用设备默认值 | `0` |

设备默认值：SpaceMouse **125 Hz**，力传感器 **1000 Hz**。

### 力传感器 — `FtSensorOptions`

由 `FtSensor::Initialize()` 消费。`sensor_type` 支持 `kunwei_serial` / `kunwei` / `xjc_serial` / `xjc`。

| 字段 | 含义 | 默认值 |
|------|------|--------|
| `serial_port` | 串口路径，如 `/dev/ttyUSB0` | — |
| `sensor_type` | 协议类型 | `xjc_serial` |
| `baud_rate` | 波特率 [bit/s] | `460800` |
| `read_buffer_size` | 单次 read 缓冲区 [byte] | `512` |
| `stale_timeout_ms` | 首帧等待基础超时 [ms]；实际超时为 `max(100, 5 × stale_timeout_ms)` | `20` |
| `sample_rate_hz` | 请求采样率 [Hz]；XJC 会映射为 100/250/500/1000 | 继承基类 `0` → 1000 |

C++ 示例：

```cpp
#include "peripherals/peripherals.hpp"

smrcore::peripherals::FtSensorOptions options;
options.serial_port = "/dev/ttyUSB0";
options.sensor_type = "kunwei_serial";
options.baud_rate = 460800;
options.sample_rate_hz = 1000.0;

smrcore::peripherals::FtSensor sensor;
sensor.Initialize(options);
sensor.Start();
sensor.WaitForFirstSample();  // 可选：等待首帧有效数据
```

### SpaceMouse — `SpaceMouseOptions`

由 `SpaceMouse::Initialize()` 消费。`device_path` 为空时自动探测设备。

| 字段 | 含义 | 默认值 |
|------|------|--------|
| `device_path` | Linux input 设备路径，如 `/dev/input/event7` | 空（自动探测） |
| `sample_rate_hz` | 输出频率 [Hz] | 继承基类 `0` → 125 |
| `axis_map` | 六轴映射（x,y,z,roll,pitch,yaw → 原始轴索引） | `{0,1,2,3,4,5}` |
| `axis_sign` | 各轴符号（<0 取反） | Y/Z/pitch/yaw 取反 |
| `axis_scale` | 归一化满量程原始计数值 | 各轴 `350` |
| `deadzone` | 死区阈值 [0, 1] | `0.01` |

C++ 示例：

```cpp
smrcore::peripherals::SpaceMouseOptions options;
options.sample_rate_hz = 125.0;
// options.device_path = "/dev/input/event7";  // 可选，留空则自动探测

smrcore::peripherals::SpaceMouse mouse;
mouse.Initialize(options);
mouse.Start();
```

### 协议层配置（扩展/二次开发）

集成方通常只需使用上述 `*Options`。若直接调用协议层 Reader，另有：

| 类型 | 位置 | 说明 |
|------|------|------|
| `SerialReaderConfig` | `protocols/ft_sensor/serial_reader.hpp` | 串口路径、波特率、read 缓冲区 |
| `KunweiReaderConfig` | `protocols/ft_sensor/kunwei/kunwei_reader.hpp` | 同 `SerialReaderConfig` |
| `XjcReaderConfig` | `protocols/ft_sensor/xjc/xjc_reader.hpp` | 继承串口配置 + `active_reporting_hz`（100/250/500/1000） |

`FtSensor` 门面会在内部将 `FtSensorOptions` 映射为对应品牌的 Reader 配置。

### 命令行覆盖

示例应用支持 CLI 参数覆盖 `*Options` 字段，无需 JSON 配置文件：

```bash
./build_Release/install/bin/app_peripherals_read_ft_sensor \
  --serial-port /dev/ttyUSB1 \
  --sensor-type kunwei_serial \
  --baud-rate 460800 \
  --sample-rate 1000 \
  --stale-timeout-ms 20 \
  --read-buffer-size 512
```

Python 示例见 `python/README.md` 与 `python/app/`。

## 编译

### 环境要求

- CMake ≥ 3.16、Conan 2.x、Ninja（Linux 默认）
- Linux x86_64 本地开发（默认）或 Linux armv8/aarch64 交叉编译；V1 暂不支持 Windows standalone 构建

### C++ 构建

```bash
# 默认构建 SDK bridge，需先下载 smrcore_sdk
./scripts/download.sh
./scripts/build.sh

# 只构建外设库、基础 app 和单元测试，不依赖 smrcore_sdk
./scripts/build.sh --with-sdk OFF --tests ON

# Debug
./scripts/build.sh -t Debug

# armv8 交叉编译（在 Docker 内执行）
rkbuild ./scripts/build.sh -a armv8

# 运行单元测试
./scripts/run_tests.sh -t Release
```

产物位于 `build_<Type>/`（`bin/`、`lib/`、`install/`）。

### 构建 SDK 桥接工具

`app_peripherals_bridge` 默认随 standalone 构建启用，依赖 `smrcore_sdk` 安装树：

```bash
./scripts/download.sh
./scripts/build.sh --with-sdk ON --sdk-root third_party/prebuilt/smrcore_sdk
```

嵌入 **rcore 主仓** 联合编译时，执行 `./scripts/build.sh -P`，rcore 会自动开启 `SMR_PERIPHERAL_WITH_SDK` 并链接同构建内的 `rcoresdk`。

### Python 绑定

先完成 C++ 构建，再：

```bash
./scripts/build_py.sh
./scripts/run_test_py.sh -t Release
```

详见 `python/README.md`。

## 示例应用（`app/`）

构建后二进制位于 `build_Release/bin/` 或 `build_Release/install/bin/`。

| 应用 | 是否需要 SDK | 功能 |
|------|:------------:|------|
| `app_peripherals_probe` | 否 | 探测 SpaceMouse 与 F/T 传感器的连接状态、型号、采样率等管理面信息 |
| `app_peripherals_read_spacemouse` | 否 | 持续打印归一化 SpaceMouse 采样（六自由度 + 夹爪状态） |
| `app_peripherals_read_spacemouse_raw` | 否 | 持续打印 SpaceMouse 原始轴计数值与按键位掩码 |
| `app_peripherals_read_ft_sensor` | 否 | 初始化力传感器、等待首帧有效数据后持续打印六维力/力矩；支持 CLI 参数 |
| `app_peripherals_bridge` | 是 | 单一 `Robot` SDK 会话内桥接 SpaceMouse 和/或 F/T 传感器；默认两路都启，适合 FDCC 双外设输入；传 `--spacemouse` 或 `--ft-sensor` 时仅启用显式选择的外设 |

未启用 SDK 时，带 `bridge` 后缀的应用会提示需在 rcore 主仓联合编译。

### 快速验证

```bash
./build_Release/install/bin/app_peripherals_probe
./build_Release/install/bin/app_peripherals_read_spacemouse
./build_Release/install/bin/app_peripherals_read_ft_sensor --serial-port /dev/ttyUSB0
```

对应 Python 示例脚本位于 `python/app/`。

## 远程仓库

```text
https://github.com/smore-robotics/smrcore_peripherals
```

## 相关仓库

- **rcore**：机器人核心库与 SDK
- **rcore-sdk-py**：Python 机器人 SDK
