# SMRCore Peripherals Examples

This repository is the public example surface for SMRCore peripheral workflows.
It does not contain SpaceMouse or F/T sensor reader source code. Examples build
against released `smrcore_peripherals` artifacts; bridge examples also require
`smrcore_sdk` and a running robot controller.

## Quick Start

```bash
./scripts/download.sh
./scripts/build.sh
```

Binaries are under `build/bin/`.

`scripts/download.sh` reads `.sdk-version` and `.peripherals-version`, then
downloads release assets from GitHub into `third_party/`.

## Read Peripherals Only (C++)

These targets link only `smrcore_peripherals`:

| Binary | Description |
|--------|-------------|
| `probe` | Probe SpaceMouse and F/T sensor connectivity and metadata |
| `read_spacemouse` | Decoded SpaceMouse samples (`--device`, `--sample-rate`) |
| `read_spacemouse_raw` | Raw HID samples (`--device`, `--sample-rate`) |
| `read_ft_sensor` | Wrench stream with full serial CLI and `WaitForFirstSample` |

```bash
./build/bin/probe --serial-port /dev/ttyUSB0
./build/bin/read_spacemouse --device /dev/input/event5 --sample-rate 125
./build/bin/read_spacemouse_raw
./build/bin/read_ft_sensor --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

## Robot SDK Bridge Examples (C++)

Built when `SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=ON` (default). Requires
`smrcore_sdk`, a reachable robot, and the peripheral connected locally.

| Binary | Description |
|--------|-------------|
| `teleop_spacemouse_sdk` | Stream SpaceMouse samples via `robot.Peripheral().UpdateSpaceMouseSample()` |
| `fdcc_external_ft_sensor` | Stream external F/T wrench via `UpdateFtSensorSample()` |
| `ft_sensor_calib_external` | Same as FDCC bridge; intended for calibration data collection |

```bash
./build/bin/teleop_spacemouse_sdk --device /dev/input/event5
./build/bin/fdcc_external_ft_sensor --serial-port /dev/ttyUSB0
```

`--robot <robot-ip>` is optional. When omitted, the example passes an empty
robot IP string to `smrcore_sdk` and lets the SDK apply its default connection
behavior.

Disable SDK examples if you only have the peripherals package:

```bash
cmake -S . -B build -DSMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=OFF \
  -DCMAKE_PREFIX_PATH=third_party/smrcore_peripherals
cmake --build build
```

## Python Examples

See `examples/python/README.md`. Read-only scripts mirror the C++ read examples;
bridge scripts are documented skeletons until Python SDK Peripheral APIs match C++.

## Repository Layout

- `examples/cpp/`: C++ examples (CMake).
- `examples/python/`: Python read examples and bridge skeletons.
- `docs/`: platform setup and workflow notes.
- `scripts/download.sh`: fetch released SDK/peripheral artifacts.
- `scripts/build.sh`: configure and build C++ examples.

## Version Files

- `.sdk-version`: expected `smrcore_sdk` release.
- `.peripherals-version`: expected `smrcore_peripherals` release.

Keep these aligned with the GitHub release versions consumed by
`scripts/download.sh`.
