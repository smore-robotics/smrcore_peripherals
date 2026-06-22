# Python Examples

Install wheels matching `.peripherals-version` (and `.sdk-version` for bridge workflows):

```bash
pip install smrcore_peripherals==$(cat ../../.peripherals-version)
# Bridge skeletons also need smrcore_sdk when Python Peripheral APIs are available:
# pip install smrcore_sdk==$(cat ../../.sdk-version)
```

Run from this directory (`examples/python/`).

## Read peripherals only

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

See `docs/platform_setup.md` for device permissions.

## Robot SDK bridge (skeleton)

| Script | C++ counterpart |
|--------|-----------------|
| `teleop_spacemouse_sdk.py` | `teleop_spacemouse_sdk` |
| `fdcc_external_ft_sensor.py` | `fdcc_external_ft_sensor` |

These exit with instructions until the Python SDK exposes the same
`robot.Peripheral().Update*Sample()` APIs as the C++ SDK. Use the C++ bridge
binaries for production teleop and FDCC external input.
