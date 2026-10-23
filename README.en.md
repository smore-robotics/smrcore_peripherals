# smrcore_peripherals

**Peripheral input and robot integration examples for SpaceMouse, force/torque sensors, Pico and master–follower arms.**

**English** · [简体中文](README.md)

The repository provides readers, protocols and application examples. Readers are independent of the robot SDK; applications combine them with the matching SDK for robot control.

## Integration chains

| Peripheral | Reader | Robot integration |
|---|---|---|
| [SpaceMouse](docs/site/spacemouse.md) | C++ / Python | SDK Peripheral sample API |
| [Force/torque sensor](docs/site/ft-sensor.md) | C++ / Python | SDK Peripheral sample API and calibration |
| [Pico](docs/site/pico.md) | C++ / Python | Dual-arm ServoP and grippers; Python can launch the C++ bridge |
| [Master–follower arms](docs/site/zhonglin.md) | Python | SDK ServoJ follower control |

## Downloads and documentation

- [Releases](https://github.com/zhanhui-li-smore/smrcore_peripherals/releases): compiled packages, wheels, bilingual manuals and checksums.
- [Asset selection and SDK compatibility](docs/releases.en.md): C++ package choices, Python dependencies and delivery scope.
- [Documentation](https://zhanhui-li-smore.github.io/smrcore_peripherals/) · [C++ examples](app/README.en.md) · [Python examples](python/app/README.en.md).
- [Maintainer release guide](docs/releasing.md): weekly checks, tags and recovery.

## Build from source

```bash
git clone https://github.com/zhanhui-li-smore/smrcore_peripherals.git
cd smrcore_peripherals
python3 -m pip install 'conan>=2,<3'
./scripts/build.sh --with-sdk OFF --tests ON
./scripts/run_tests.sh -t Release
./build_Release/bin/app_peripherals_probe
```

Build and install the Python reader wheel after C++:

```bash
./scripts/build_py.sh --no-install
python3 -m pip install --force-reinstall python/dist/*.whl
```

Prebuilt packages are also available. For robot applications follow the relevant chain guide to install the SDK and verify the mounting configuration and device settings.

## License and operation

Repository code uses [Apache License 2.0](LICENSE). SDK runtimes and document templates retain their own attribution and licenses. Readers need device permissions; motion applications require a checked tool, workspace and emergency stop. See [release information](docs/releases.en.md) for platforms, SDK versions and validation scope.
