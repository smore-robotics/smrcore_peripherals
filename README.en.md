# smrcore_peripherals

**Read SpaceMouse, force/torque sensors and Pico, and integrate robot applications with the Smartmore Robot SDK.**

**English** · [简体中文](README.md)

---

The public source repository for the Smartmore peripheral SDK. It provides a C++ library, Python wheels, reader examples, and optional robot SDK bridges. Readers do not connect to robots; separate bridges compose them with [smrcore_sdk](https://github.com/smore-robotics/smrcore_sdk).

## Documentation

- [在线文档 / Documentation site](https://smore-robotics.github.io/smrcore_peripherals/zh/getting-started/)（组织站点部署后可用 / available after upstream deployment）

- [C++ applications](app/README.en.md)
- [Python applications](python/app/README.en.md)
- [Platform setup](docs/platform_setup.en.md)
- [Release assets, SDK compatibility and publishing](docs/releases.en.md)
- [Releases](https://github.com/smore-robotics/smrcore_peripherals/releases): tagged builds include English and Chinese PDF / HTML manuals.

## Quick Start

### C++

Linux x86_64; Ubuntu 22.04 is recommended. Install GCC/CMake, Conan 2 and Python 3.10+.

```bash
git clone https://github.com/smore-robotics/smrcore_peripherals.git
cd smrcore_peripherals
python3 -m pip install 'conan>=2,<3'
./scripts/build.sh --with-sdk OFF --tests ON
./scripts/run_tests.sh -t Release
./build_Release/bin/app_peripherals_probe
./build_Release/bin/app_peripherals_read_pico --port 8765
```

### Python

The wheel reads peripherals only; it does not bundle the robot SDK or teleoperation core.

```bash
./scripts/build_py.sh --no-install
python3 -m pip install --force-reinstall python/dist/*.whl
python3 python/app/app_peripherals_read_pico.py --port 8765
```

Release wheels target CPython 3.10, 3.11 and 3.12 on Linux x86_64. Untagged development wheels use version 0.0.0; identify them by their Actions commit.

### Robot SDK bridge

```bash
./scripts/download.sh
./scripts/build.sh --with-sdk ON --with-pico OFF
```

[.sdk-version](.sdk-version) pins **0.2.0** for the original SpaceMouse/F/T bridges. See application documentation before running them.

The optional Pico dual-arm bridge requires `--with-pico ON --sdk-root /path/to/compatible-sdk`. The original official v0.2.0 package omits `sdk/dual_arm_robot.hpp`; use a corrected package with matching headers and libraries. This example **embeds B3 mounting-bracket transforms for the corresponding B3 dual-arm configuration**, not a generic dual-arm setup. Hardware acceptance is pending; default release binaries exclude this bridge.

## Release Assets

| Asset | Description |
|---|---|
| `smrcore_peripherals-cpp-linux-x86_64-v<version>.tar.gz` | C++ readers, headers, library, CMake config and Python examples |
| `smrcore_peripherals-sdk-bridge-linux-x86_64-v<version>.tar.gz` | Original SpaceMouse/F/T bridges, calibration tools and SDK runtime |
| `rcore_peripherals_py-<version>-cp3xx-cp3xx-linux_x86_64.whl` | CPython 3.10–3.12 reader wheels |
| `smrcore_peripherals-manual-zh-v<version>.pdf/html` | Chinese manual |
| `smrcore_peripherals-manual-en-v<version>.pdf/html` | English manual |
| `SHA256SUMS` | Tagged release checksums |

Every push/PR builds [Actions artifacts](https://github.com/smore-robotics/smrcore_peripherals/actions). Successful `vX.Y.Z` tag builds publish a GitHub Release. This table describes the configured outputs, not assets already published before the first successful CI run. Windows, ARM64 and simulator artifacts are not currently promised.

After extracting a C++ archive, use `./run.sh app_peripherals_read_pico --port 8765`; the wrapper resolves bundled runtime libraries. Device permissions are still required.

## Safety

Robot bridges affect physical hardware. Verify the robot, tool, payload, workspace and example configuration, and keep the emergency stop reachable. Build and simulation checks do not establish hardware acceptance.

## License

[Apache License 2.0](LICENSE). SDK bridge archives preserve the SDK's bundled third-party licenses and attribution.
