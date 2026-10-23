# Force/torque sensor

Device input → peripheral reader → robot SDK Peripheral API → cabinet control.

## Read input

```bash
python3 python/app/app_peripherals_read_ft_sensor.py --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

## Robot integration

```bash
python3 python/app/app_peripherals_bridge.py --robot-ip <robot-ip> --ft-sensor
```

Install the peripherals wheel and robot Python SDK. The bridge uploads samples; cabinet configuration determines the resulting motion. C++ entries and SDK build steps are in [robot integration](robot-integration.md).

## Configuration

Choose the device node, protocol and baud rate for your hardware. See [platform setup](platform.md).

Static calibration: [Python examples](python.md).
