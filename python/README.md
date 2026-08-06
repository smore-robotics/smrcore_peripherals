# rcore-peripherals-py

面向 SMRCore 外设 SDK 的 Python 绑定，基于 pybind11 + scikit-build-core 构建（Python >= 3.10）。

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

对应 C++ `app/` 的 Python 脚本位于 `python/app/`。

仅读外设（只需 `rcore-peripherals-py`）：

```bash
python3 python/app/app_peripherals_read_ft_sensor.py
python3 python/app/app_peripherals_read_spacemouse.py
```

## 机器人 SDK（bridge 依赖）

`app_peripherals_bridge.py` 除本仓库构建的 `rcore-peripherals-py` 外，还需要**额外**从 [smrcore_sdk](https://github.com/smore-robotics/smrcore_sdk) GitHub Releases 安装机器人 Python SDK（包名 `rcore-sdk-py`，import 名 `rcore_sdk`）。行为与 C++ `app_peripherals_bridge` 一致。

**版本须与仓库根目录 `.sdk-version` 一致**（当前为 `0.0.25`，与 C++ `./scripts/download.sh` 下载的 `smrcore_sdk-cpp-*` 同源，不要混用不同版本）。

发布页：https://github.com/smore-robotics/smrcore_sdk/releases/tag/v0.0.25（候选包见 `prerelease` release）

在 release **Assets** 中选择与当前 Python ABI、平台匹配的 `rcore_sdk_py-0.0.25-<python-tags>.whl`，例如 Linux x86_64 + CPython 3.10 为 `cp310-cp310-linux_x86_64`。

```bash
PY_TAG=cp310-cp310-linux_x86_64   # 其他平台见 release Assets，如 cp310-cp310-win_amd64
VERSION=0.0.25
RELEASE_TAG=prerelease            # 正式版发布后改为 v${VERSION}

curl -L --fail \
  "https://github.com/smore-robotics/smrcore_sdk/releases/download/${RELEASE_TAG}/rcore_sdk_py-${VERSION}-${PY_TAG}.whl" \
  -o "rcore_sdk_py-${VERSION}-${PY_TAG}.whl"
python3 -m pip install "./rcore_sdk_py-${VERSION}-${PY_TAG}.whl"
```

安装外设 wheel 与机器人 SDK 后运行 bridge：

```bash
python3 -m pip install --force-reinstall python/dist/*.whl   # 若尚未安装 rcore-peripherals-py
python3 python/app/app_peripherals_bridge.py --robot <robot-ip>
```

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
    sensor_type="xjc_serial",
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
