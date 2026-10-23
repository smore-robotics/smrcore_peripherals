# C++ Examples

Build the readers without a robot SDK:

```bash
./scripts/build.sh --with-sdk OFF --tests ON
```

| Program | Purpose |
| --- | --- |
| app_peripherals_probe | Detect input devices |
| app_peripherals_read_spacemouse_raw | Inspect raw SpaceMouse input |
| app_peripherals_read_pico | Read Pico poses and buttons |
| app_peripherals_bridge | Optional robot SDK sample bridge |
| app_peripherals_pico_bridge | Optional B3 dual-arm teleoperation |

Use `--help` for executable options. [Robot integration](robot-integration.md) describes optional build requirements. [Full C++ application guide](https://github.com/smore-robotics/smrcore_peripherals/blob/main/app/README.en.md).
