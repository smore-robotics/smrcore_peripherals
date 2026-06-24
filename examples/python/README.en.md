# Python Examples

**English** · [简体中文](README.md)

The current source repository can build the `rcore_peripherals` Python wheel directly. Example scripts live in this directory and under `python/app/`; both use the same Python API.

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
| `probe.py` | Print SpaceMouse and F/T sensor model, rate, and connection flags |
| `read_spacemouse.py` | Stream decoded 6-DOF + gripper samples |
| `read_spacemouse_raw.py` | Stream raw axis/button samples |
| `read_ft_sensor.py` | Stream wrench samples after the first valid frame |

```bash
python3 examples/python/probe.py --serial-port /dev/ttyUSB0
python3 examples/python/read_spacemouse.py --device /dev/input/event5 --sample-rate 125
python3 examples/python/read_spacemouse_raw.py --device /dev/input/event5
python3 examples/python/read_ft_sensor.py \
  --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

Scripts under `python/app/` are maintained with the installed package:

```bash
python3 python/app/app_peripherals_read_spacemouse.py
python3 python/app/app_peripherals_read_ft_sensor.py --serial-port /dev/ttyUSB0
```

## Robot SDK Bridge

Python bridge script `python/app/app_peripherals_bridge.py` requires both the peripherals wheel and the robot SDK Python package. If the current Python SDK does not yet expose the same `Peripheral().Update*Sample()` APIs as C++, use C++ `app_peripherals_bridge` for production bridge workflows.

## Safety

> Robots are hazardous machines. Before running bridge scripts, verify the workspace is clear, the emergency stop is reachable, and peripheral input cannot cause unintended motion.
