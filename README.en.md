<div align="center">

# smrcore_peripherals

**Probe, read, and bridge SpaceMouse and F/T sensors on the SMRCore peripherals library.**

[![License](https://img.shields.io/badge/License-Apache%202.0-1f6feb.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/smore-robotics/smrcore_peripherals?label=Release&color=1f6feb)](https://github.com/smore-robotics/smrcore_peripherals/releases)

**English** · [简体中文](README.md)

</div>

---

The public integration repository for **SMRCore peripherals**. It ships build
scripts, runnable C++ and Python examples, and release download helpers for the
prebuilt C++ package and Python wheel. It does **not** contain peripheral reader
source code (SpaceMouse and F/T sensor drivers live in the internal
`rcore/peripheral` module).

Bridge examples also require
[`smrcore_sdk`](https://github.com/smore-robotics/smrcore_sdk) and a reachable
robot controller.

## Documentation

| Doc | Description |
|---|---|
| [C++ examples](examples/cpp/README.en.md) | Build, read-only binaries, and SDK bridge targets |
| [Python examples](examples/python/README.en.md) | Wheel install and script usage |
| [Platform setup](docs/platform_setup.en.md) | SpaceMouse event nodes, serial access, robot connectivity |

## Quick Start

```bash
git clone https://github.com/smore-robotics/smrcore_peripherals.git
cd smrcore_peripherals

./scripts/download.sh    # fetch releases pinned by .peripherals-version / .sdk-version
./scripts/build.sh       # build C++ examples (see examples/cpp/README.en.md)

./build/bin/probe
```

Python examples require a separate wheel install — see
[examples/python/README.en.md](examples/python/README.en.md).

## Repository Layout

| Path | Contents |
|---|---|
| `examples/cpp/` | C++ examples (CMake) |
| `examples/python/` | Python read scripts and SDK bridge skeletons |
| `docs/` | Platform setup notes |
| `scripts/download.sh` | Download SDK / peripheral release assets |
| `scripts/build.sh` | Configure and build C++ examples |

## Version Files

| File | Meaning |
|---|---|
| `.peripherals-version` | Expected `smrcore_peripherals` / `rcore-peripherals-py` release |
| `.sdk-version` | Expected `smrcore_sdk` / `rcore-sdk-py` release for bridge examples |

Keep these aligned with the GitHub release versions consumed by
`scripts/download.sh`.

## Release Assets

Assets are published on the
**[Releases page](https://github.com/smore-robotics/smrcore_peripherals/releases)**:

| Asset | Description |
|---|---|
| `rcore_peripherals-cpp-linux-x86_64-v<version>.tar.gz` | C++ peripherals library for Linux x86_64 |
| `rcore_peripherals-cpp-windows-x86_64-v<version>.tar.gz` | C++ peripherals library for Windows x86_64 |
| `rcore_peripherals_py-<version>-<python-tags>.whl` | Python wheel (per Python ABI / platform) |

Bridge workflows also need `smrcore_sdk-cpp-*` and `rcore_sdk_py-*` assets from
[smrcore_sdk Releases](https://github.com/smore-robotics/smrcore_sdk/releases).

## Safety

> Robots are hazardous machines. Before running any bridge or motion-related
> example, verify the workspace is clear, the emergency stop is reachable, and
> peripheral input cannot cause unintended motion. F/T and teleop data directly
> affect controller behavior.

## License

This repository (examples, scripts, and docs) is released under the
[Apache License 2.0](LICENSE). Prebuilt peripheral release artifacts bundle
third-party components whose license and attribution notices ship inside each
release archive.

<div align="center">

Copyright © Smartmore Corporation

</div>
