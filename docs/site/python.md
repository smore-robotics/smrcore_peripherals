# Python Examples

The reader wheel imports as `rcore_peripherals` and does not require the robot SDK.

| Script | Purpose |
| --- | --- |
| app_peripherals_read_spacemouse.py | Read SpaceMouse samples |
| app_peripherals_read_ft_sensor.py | Read force/torque samples |
| app_peripherals_read_pico.py | Print Pico positions and buttons |
| app_peripherals_bridge.py | Optional sample bridge; requires compatible robot Python SDK APIs |
| app_peripherals_ft_sensor_calib.py | Robot-based F/T calibration; requires robot SDK |

Start with [installation](getting-started.md) and [reader examples](readers.md). A separate Python robot SDK is required for the last two scripts. If its Peripheral update APIs are unavailable, use the C++ bridge. The wheel does not provide Pico robot teleoperation.
