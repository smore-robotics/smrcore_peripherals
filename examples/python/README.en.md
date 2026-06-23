# Python Examples

**English** · [简体中文](README.md)

Install wheels from GitHub Releases (versions should match `.peripherals-version` and,
for bridge workflows, `.sdk-version`):

```bash
VERSION=0.0.1  # match ../../.peripherals-version
PY_TAG=cp310-cp310-linux_x86_64   # Windows: cp310-cp310-win_amd64
curl -L -O "https://github.com/smore-robotics/smrcore_peripherals/releases/download/v${VERSION}/rcore_peripherals_py-${VERSION}-${PY_TAG}.whl"
python3 -m pip install "./rcore_peripherals_py-${VERSION}-${PY_TAG}.whl"
```

Run from this directory (`examples/python/`).

PyPI-style package name: `rcore-peripherals-py`; import name: `rcore_peripherals`.

Platform permissions and device setup:
[Platform setup](../../docs/platform_setup.en.md).

## Read Peripherals Only

| Script | Purpose |
|--------|---------|
| `probe.py` | Print SpaceMouse and F/T sensor model, rate, and connected flags |
| `read_spacemouse.py` | Stream decoded 6-DOF + gripper samples |
| `read_spacemouse_raw.py` | Stream raw HID axis/button samples |
| `read_ft_sensor.py` | Stream wrench samples after first valid frame |

### Usage

```bash
python3 probe.py
python3 probe.py --serial-port /dev/ttyUSB1

python3 read_spacemouse.py
python3 read_spacemouse.py --device /dev/input/event5 --sample-rate 125

python3 read_spacemouse_raw.py --device /dev/input/event5

python3 read_ft_sensor.py --serial-port /dev/ttyUSB0 --sensor-type xjc_serial \
  --baud-rate 460800 --sample-rate 1000
```

## Robot SDK Bridge

| Script | C++ counterpart |
|--------|-----------------|
| `peripherals_sdk.py` | `peripherals_sdk` |

Install both wheels (versions from repo root):

```bash
VERSION=0.0.1       # match ../../.peripherals-version
SDK_VERSION=0.0.3   # match ../../.sdk-version
PY_TAG=cp310-cp310-linux_x86_64   # Windows: cp310-cp310-win_amd64

curl -L -O "https://github.com/smore-robotics/smrcore_peripherals/releases/download/v${VERSION}/rcore_peripherals_py-${VERSION}-${PY_TAG}.whl"
python3 -m pip install "./rcore_peripherals_py-${VERSION}-${PY_TAG}.whl"

curl -L -O "https://github.com/smore-robotics/smrcore_sdk/releases/download/v${SDK_VERSION}/rcore_sdk_py-${SDK_VERSION}-${PY_TAG}.whl"
python3 -m pip install "./rcore_sdk_py-${SDK_VERSION}-${PY_TAG}.whl"
```

### Usage

```bash
# FDCC: both SpaceMouse and F/T sensor (edit device constants in the script first)
python3 peripherals_sdk.py
python3 peripherals_sdk.py --robot 192.168.1.100

# Teleop only or F/T only
python3 peripherals_sdk.py --spacemouse
python3 peripherals_sdk.py --ft-sensor
python3 peripherals_sdk.py --spacemouse --ft-sensor
```

`--robot` is optional; when omitted the SDK uses its default local connection behavior.
Device paths and serial settings are constants at the top of `peripherals_sdk.py`
(mirror of the C++ example). For CLI-driven device options use
`rcore/peripheral/python/app/app_peripherals_bridge.py` instead.

## Safety

> Robots are hazardous machines. Before running bridge scripts, verify the
> workspace is clear, the emergency stop is reachable, and peripheral input
> cannot cause unintended motion.
