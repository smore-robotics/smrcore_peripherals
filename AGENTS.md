# AGENTS.md - smrcore_peripherals

Public source repository for SMRCore peripheral integration. It contains the C++/Python peripheral reader implementation, diagnostic apps, examples, and the SDK bridge entry points.

## Layout

| Path | What |
|---|---|
| `src/` | Public C++ API, protocol parsers, and device IO |
| `app/` | Recommended C++ tools: probe, read, and bridge |
| `python/` | pybind11 wheel, Python scripts, and tests |
| `examples/` | Public example source and bilingual usage docs |
| `docs/` | Platform setup notes |
| `scripts/` | Download, build, test, and package helpers |
| `.sdk-version` | Robot SDK version used by standalone bridge builds |

## Common Tasks

```bash
./scripts/build.sh --with-sdk OFF --tests ON
./scripts/run_tests.sh -t Release

./scripts/download.sh
./scripts/build.sh --with-sdk ON --tests ON
./build_Release/bin/app_peripherals_bridge --robot <robot-ip>
```

## C++ Conventions

- C++17.
- Function and method names use PascalCase.
- Local variables and parameters use snake_case.
- High-rate robot injection goes through `robot.Peripheral().UpdateSpaceMouseSample()` and `robot.Peripheral().UpdateFtSensorSample()`.
