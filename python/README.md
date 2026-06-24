# rcore-peripherals-py

面向 rcore 外设 SDK 的 Python 绑定，基于 pybind11 + scikit-build-core 构建，设置与 `rcore-sdk-py` 对齐（Python >= 3.10）。

## C++ 依赖

本地开发需先完成 C++ 构建，再指向安装目录：

```bash
./scripts/build.sh
export SMRCORE_PERIPHERALS_CPP_ROOT=../build_Release/install
cd python
python3 -m pip install -v --no-deps -e .
```

## 构建与测试

先编译 C++，再单独构建 Python 绑定并运行测试：

```bash
./scripts/build.sh
./scripts/build_py.sh
./scripts/run_test_py.sh
```

`build_py.sh` 默认使用 `build_Release/install` 作为 C++ 安装前缀；也可通过 `--cpp-root` 覆盖。

底层 wheel 逻辑在 `python/scripts/build_wheel.sh`（由 `build_py.sh` 调用）。

wheel 会打包 `libsmrcore_peripherals.so` 到 `rcore_peripherals/.libs/`，干净环境中无需单独配置 `LD_LIBRARY_PATH`。

## 示例应用

对应 C++ `app/` 的 Python 脚本位于 `python/app/`：

```bash
python3 python/app/app_peripherals_read_ft_sensor.py
python3 python/app/app_peripherals_read_spacemouse.py
python3 python/app/app_peripherals_bridge.py --robot <robot-ip>
```

`app_peripherals_bridge.py` 需额外安装 `rcore-sdk-py`（import 名 `rcore_sdk`），行为与 C++ `app_peripherals_bridge` 一致。

## API

与 C++ `smrcore::peripherals` 对齐：

- **类型**：`PeripheralOptions`、`FtSensorOptions`、`SpaceMouseOptions`、`PeripheralInfo`、`SpaceMouseSample`、`SpaceMouseRawSample`、`FtSensorSample`、`GripperCommand`
- **外设**：`FtSensor`、`SpaceMouse`（`Initialize` / `Shutdown` / `Start` / `Stop` / `IsConnected` / `GetInfo` / `GetSample`；SpaceMouse 另有 `GetRawSample`）

示例：

```python
from rcore_peripherals import FtSensor, FtSensorOptions

sensor = FtSensor()
options = FtSensorOptions(
    serial_port="/dev/ttyUSB0",
    sensor_type="kunwei_serial",
    baud_rate=460800,
    sample_rate_hz=1000.0,
    stale_timeout_ms=20,
)
if not sensor.Initialize(options):
    raise RuntimeError("failed to initialize F/T peripheral")
if not sensor.Start() or not sensor.WaitForFirstSample():
    raise RuntimeError("failed to start F/T peripheral")

while True:
    sample = sensor.GetSample()
    if sample is not None:
        print(sample.fx, sample.fy, sample.fz, sample.tx, sample.ty, sample.tz)
```

SpaceMouse 使用 `SpaceMouseOptions` 初始化；对应 C++ `app_peripherals_read_spacemouse.cpp` 见 `app/app_peripherals_read_spacemouse.py`。
