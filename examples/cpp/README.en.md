# C++ Examples

**English** · [简体中文](README.md)

C++ examples built from the repository root via CMake. They consume the
`smrcore_peripherals` C++ package. Bridge targets also require
[`smrcore_sdk`](https://github.com/smore-robotics/smrcore_sdk) and a reachable
robot controller.

For platform permissions and device setup, see
[Platform setup](../../docs/platform_setup.en.md).

## Build

From the repository root:

```bash
./scripts/download.sh    # fetch releases pinned by .peripherals-version / .sdk-version
./scripts/build.sh       # configure and build C++ examples
```

Binaries live under `build/bin/`. `scripts/download.sh` pulls release assets into
`third_party/`.

If you only have the peripherals C++ package and do not need SDK bridge examples:

```bash
cmake -S . -B build -DSMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=OFF \
  -DCMAKE_PREFIX_PATH=third_party/smrcore_peripherals
cmake --build build
```

Or:

```bash
SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=OFF ./scripts/build.sh
```

## Read Peripherals Only

These targets link only the `smrcore_peripherals` C++ package:

| Binary | Description |
|---|---|
| `probe` | Probe SpaceMouse and F/T sensor model, rate, and connection flags |
| `read_spacemouse` | Decoded SpaceMouse 6-DOF + gripper samples (`--device`, `--sample-rate`) |
| `read_spacemouse_raw` | Raw HID axis/button samples |
| `read_ft_sensor` | Serial wrench stream with `WaitForFirstSample` and full serial CLI |

```bash
./build/bin/probe
./build/bin/probe --serial-port /dev/ttyUSB0

./build/bin/read_spacemouse --device /dev/input/event5 --sample-rate 125
./build/bin/read_spacemouse_raw
./build/bin/read_ft_sensor --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

## Robot SDK Bridge

Built when `SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=ON` (default). Requires
`smrcore_sdk`, a reachable robot, and locally connected peripherals.

| Binary | Description |
|---|---|
| `peripherals_sdk` | Push SpaceMouse and/or external F/T data via `robot.Peripheral().Update*Sample()` (both enabled by default for FDCC) |

```bash
# FDCC: SpaceMouse + F/T (edit device constants at the top of the .cpp first)
./build/bin/peripherals_sdk
./build/bin/peripherals_sdk --robot <robot-ip>

# Teleop only or F/T only
./build/bin/peripherals_sdk --spacemouse
./build/bin/peripherals_sdk --ft-sensor
```

`--robot <robot-ip>` is optional. When omitted, the example passes an empty IP
and lets the SDK use its default local connection behavior. Start the bridge
**before** enabling Teleoperation or FDCC so rcore sees fresh external input.

## Safety

> Robots are hazardous machines. Before running bridge examples, verify the
> workspace is clear, the emergency stop is reachable, and peripheral input
> cannot cause unintended motion. F/T and teleop data directly affect controller
> behavior.
