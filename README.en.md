<div align="center">

# smrcore_peripherals

**Build the SMRCore peripherals library from source, probe/read SpaceMouse and F/T sensors, and bridge samples into rcore through the robot SDK.**

[![License](https://img.shields.io/badge/License-Apache%202.0-1f6feb.svg)](LICENSE)

**English** · [简体中文](README.md)

</div>

---

`smrcore_peripherals` is the public source repository for SMRCore peripheral support. It contains local SpaceMouse and force/torque sensor readers, protocol parsers, sample normalization, C++/Python APIs, diagnostic tools, and SDK bridge applications.

This repository does not depend on rcore source code and does not maintain robot IDL. To send peripheral data into the robot controller, use [`smrcore_sdk`](https://github.com/smore-robotics/smrcore_sdk) and call `Robot::Peripheral().UpdateSpaceMouseSample()` / `UpdateFtSensorSample()`. High-rate peripheral data does not go through RPC.

## Documentation

| Doc | Description |
|---|---|
| [C++ examples](examples/cpp/README.en.md) | Build and run the in-tree apps and legacy examples |
| [Python examples](examples/python/README.en.md) | Build/install the Python wheel and run scripts |
| [Platform setup](docs/platform_setup.en.md) | SpaceMouse event nodes, serial permissions, robot connectivity |

## Quick Start

Build only the peripherals library, basic apps, and tests. This path does not require the robot SDK:

```bash
git clone https://github.com/smore-robotics/smrcore_peripherals.git
cd smrcore_peripherals

./scripts/build.sh --with-sdk OFF --tests ON
./scripts/run_tests.sh -t Release

./build_Release/bin/app_peripherals_probe
```

To build the SDK bridge app, first download the `smrcore_sdk` version pinned by `.sdk-version`:

```bash
./scripts/download.sh
./scripts/build.sh --with-sdk ON --tests ON

./build_Release/bin/app_peripherals_bridge --robot <robot-ip>
```

## Features

| Module | Description |
|---|---|
| SpaceMouse | Reads Linux input events and outputs normalized 6-DOF + gripper target samples |
| F/T Sensor | Supports Kunwei `kunwei_serial` and XJC `xjc_serial` serial protocols |
| C++ API | `smrcore::peripherals` namespace with `Initialize` → `Start` → `GetSample` → `Stop` → `Shutdown` lifecycle |
| Python API | `rcore_peripherals` wheel aligned with the C++ facade |
| SDK bridge | `app_peripherals_bridge` injects SpaceMouse / F/T samples into rcore through `smrcore_sdk` |

## Repository Layout

| Path | Contents |
|---|---|
| `src/` | Public API, internal helpers, protocol parsers, and device IO |
| `app/` | Recommended C++ tools: probe, read, and bridge |
| `python/` | pybind11 Python wheel, scripts, and tests |
| `examples/` | Public example source; kept semantically aligned with `app/` |
| `docs/` | Platform setup notes |
| `scripts/download.sh` | Downloads `smrcore_sdk` only; it does not download this repository's own binaries |
| `scripts/build.sh` | Builds the C++ library, apps, tests, and optional SDK bridge |

Installed public headers are available under `<peripherals/...>`:

```cpp
#include "peripherals/peripherals.hpp"
```

External consumers can use:

```cmake
find_package(smrcore_peripherals CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE smrcore::peripherals)
```

## Version Files

| File | Meaning |
|---|---|
| `.sdk-version` | Default `smrcore_sdk` version for standalone bridge builds |

`scripts/download.sh` reads `.sdk-version` and downloads `smrcore_sdk-cpp-linux-<arch>-v<version>.tar.gz` into `third_party/smrcore_sdk/`. V1 does not support standalone Windows download/build.

## Common Commands

```bash
# Lightweight build without SDK bridge
./scripts/build.sh --with-sdk OFF --tests ON

# Download SDK and build the bridge
./scripts/download.sh
./scripts/build.sh --with-sdk ON --tests ON

# Run C++ tests
./scripts/run_tests.sh -t Release

# Build and verify the Python wheel
./scripts/build_py.sh
./scripts/run_test_py.sh -t Release
```

## Safety

> Robots are hazardous machines. Before running any bridge or motion-related example, verify the workspace is clear, the emergency stop is reachable, and peripheral input cannot cause unintended motion. F/T and teleop data directly affect controller behavior.

## License

This repository is released under the [Apache License 2.0](LICENSE). Third-party dependencies remain under their respective licenses.

<div align="center">

Copyright © Smartmore Corporation

</div>
