# AGENTS.md - smrcore_peripherals

Public examples and workflow notes for SMRCore peripheral integration. This
repository does not contain peripheral reader source code; it consumes versioned
release assets for `smrcore_peripherals` and, for robot bridge examples,
`smrcore_sdk`.

## Layout

| Path | What |
|---|---|
| `examples/cpp/` | C++ examples for probing, reading, and injecting samples into the robot SDK |
| `examples/python/` | Python examples for read-only peripheral workflows plus bridge skeletons |
| `docs/` | Setup, dependency, and workflow notes |
| `scripts/` | Download and build helpers |
| `.peripherals-version` | Peripheral SDK version (`x.y.z` or `latest`) targeted by examples |
| `.sdk-version` | Robot SDK version (`x.y.z` or `latest`) targeted by bridge examples |

## Common tasks

```bash
./scripts/download.sh
./scripts/build.sh
./build/bin/probe
./build/bin/read_ft_sensor --serial-port /dev/ttyUSB0
./build/bin/teleop_spacemouse_sdk
```

Use `SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES=OFF ./scripts/build.sh` when only
the peripheral package is available.

## Release assets

Expected GitHub Release assets:

- `smrcore_peripherals-cpp-linux-x86_64-v<version>.tar.gz`
- `smrcore_peripherals-cpp-windows-x86_64-v<version>.tar.gz`
- `smrcore_peripherals_py-<version>-<python-tags>.whl`

## C++ conventions

- C++17.
- Function and method names use PascalCase.
- Local variables and parameters use snake_case.
- Do not add DDS publishing examples here; robot injection goes through
  `robot.Peripheral().UpdateSpaceMouseSample()` and
  `robot.Peripheral().UpdateFtSensorSample()`.
