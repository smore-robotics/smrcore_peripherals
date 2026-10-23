# SpaceMouse

Device input → peripheral reader → robot SDK Peripheral API → cabinet control.

## Read input

```bash
python3 python/app/app_peripherals_read_spacemouse.py --device /dev/input/event5 --sample-rate 125
```

## Robot integration

```bash
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip> --spacemouse
```

Install the peripherals wheel and robot Python SDK. The bridge uploads samples; cabinet configuration determines the resulting motion. C++ entries and SDK build steps are in [robot integration](robot-integration.md).

## Configuration

Choose the device node, protocol and baud rate for your hardware. See [platform setup](platform.md).
