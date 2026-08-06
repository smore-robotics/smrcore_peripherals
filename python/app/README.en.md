# Python Applications

**English** · [简体中文](README.md)

This directory contains Python scripts aligned with the C++ `app/` tools, using the `rcore_peripherals` Python API.

For platform permissions and device setup, see [Platform setup](../../docs/platform_setup.en.md).

## Build and Install the Wheel

From the repository root:

```bash
./scripts/build.sh --with-sdk OFF --tests OFF
./scripts/build_py.sh
python3 -m pip install --force-reinstall python/dist/*.whl
```

Distribution name: `rcore-peripherals-py`; import name: `rcore_peripherals`.

## Read Peripherals Only

| Script | Purpose |
|---|---|
| `app_peripherals_read_spacemouse.py` | Stream decoded 6-DOF + gripper samples |
| `app_peripherals_read_ft_sensor.py` | Stream wrench samples after the first valid frame |
| `app_peripherals_bridge.py` | Push SpaceMouse / F/T samples into the robot SDK |
| `app_peripherals_ft_sensor_calib.py` | MoveJ static F/T calibration (Preview / `--save`) |

```bash
python3 python/app/app_peripherals_read_spacemouse.py --device /dev/input/event5 --sample-rate 125
python3 python/app/app_peripherals_read_ft_sensor.py \
  --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

For probe and raw SpaceMouse sampling, use the C++ `app_peripherals_probe` and `app_peripherals_read_spacemouse_raw` tools. See [app/README.en.md](../../app/README.en.md).

## Robot SDK Bridge

`app_peripherals_bridge.py` requires both the peripherals wheel and the robot SDK Python package (`rcore-sdk-py`). If the current Python SDK does not yet expose the same `Peripheral().Update*Sample()` APIs as C++, use C++ `app_peripherals_bridge` for production bridge workflows.

```bash
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip>
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip> --spacemouse
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip> --ft-sensor
```

See [python/README.md](../README.md) for robot SDK installation notes.

## Static F/T Calibration

`app_peripherals_ft_sensor_calib.py` mirrors C++ `app_peripherals_ft_sensor_calib` and depends only on `rcore-sdk-py` (it does not open the serial port). Start a bridge first so raw samples are available.

```bash
# 1) Inject raw (Python or C++ bridge)
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip> --ft-sensor

# 2) Preview; use --save only after Preview looks good
python3 python/app/app_peripherals_ft_sensor_calib.py --robot-ip <robot-ip>
python3 python/app/app_peripherals_ft_sensor_calib.py --robot-ip <robot-ip> --save
```

CLI matches C++: `--robot-ip`, `--nominal`, `--settle-ms`, `--sample-ms`, `--velocity-ratio`, `--joint0-deg`, `--joint5-deg`, `--save`.

## Safety

> Robots are hazardous machines. Before running bridge or calibration scripts, verify the workspace is clear, the emergency stop is reachable, and peripheral input cannot cause unintended motion. Force control requires a one-time `--save` calibration first.
