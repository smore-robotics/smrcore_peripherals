# C++ Applications

**English** · [简体中文](README.md)

This directory contains peripheral diagnostic tools and SDK bridge executables using the `smrcore::peripherals` C++ API.

For platform permissions and device setup, see [Platform setup](../docs/platform_setup.en.md).

## Build

From the repository root:

```bash
# No robot SDK required: build the peripherals library, read apps, and tests
./scripts/build.sh --with-sdk OFF --tests ON

# Required for app_peripherals_bridge
./scripts/download.sh
./scripts/build.sh --with-sdk ON --tests ON
```

Binaries are built under `build_Release/bin/`:

| Binary | Description |
|---|---|
| `app_peripherals_probe` | Probe SpaceMouse and F/T sensor model, rate, and connection flags |
| `app_peripherals_read_spacemouse` | Decoded SpaceMouse 6-DOF + gripper target samples |
| `app_peripherals_read_spacemouse_raw` | Raw Linux input axis/button samples |
| `app_peripherals_read_ft_sensor` | Serial wrench stream with `WaitForFirstSample` and full serial CLI |
| `app_peripherals_bridge` | Push SpaceMouse and/or F/T data via `robot.Peripheral().Update*Sample()` |

## Read Peripherals Only

```bash
./build_Release/bin/app_peripherals_probe
./build_Release/bin/app_peripherals_probe --serial-port /dev/ttyUSB0

./build_Release/bin/app_peripherals_read_spacemouse --device /dev/input/event5 --sample-rate 125
./build_Release/bin/app_peripherals_read_spacemouse_raw --device /dev/input/event5
./build_Release/bin/app_peripherals_read_ft_sensor \
  --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

## Robot SDK Bridge

`app_peripherals_bridge` requires `smrcore_sdk`, a reachable robot, and locally connected peripherals.

```bash
# Both SpaceMouse and F/T sensor are enabled by default, useful for FDCC
./build_Release/bin/app_peripherals_bridge --robot <robot-ip>

# Teleop only or F/T only
./build_Release/bin/app_peripherals_bridge --robot <robot-ip> --spacemouse
./build_Release/bin/app_peripherals_bridge --robot <robot-ip> --ft-sensor

# Override device options
./build_Release/bin/app_peripherals_bridge --robot <robot-ip> \
  --spacemouse-device /dev/input/event5 \
  --ft-serial-port /dev/ttyUSB0 \
  --ft-sensor-type xjc_serial
```

`--robot <robot-ip>` is optional. When omitted, the app passes an empty IP and lets the SDK apply its default connection behavior. Start the bridge before enabling Teleoperation or FDCC so the controller receives fresh peripheral samples.

## Safety

> Robots are hazardous machines. Before running bridge apps, verify the workspace is clear, the emergency stop is reachable, and peripheral input cannot cause unintended motion. F/T and teleop data directly affect controller behavior.
