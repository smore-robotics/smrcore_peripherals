# Workflows

## Probe Hardware

Use `probe` (or `examples/python/probe.py`) to print model, sample rate, and
connection flags for SpaceMouse and F/T sensor without starting a long read loop.
Pass `--serial-port` when the F/T device is not on `/dev/ttyUSB0`.

## Read A Peripheral

Use the read examples to verify local permissions and sample conversion before
involving the robot:

| Example | Use when |
|---------|----------|
| `read_spacemouse` | Normal teleop-style decoded axes and gripper |
| `read_spacemouse_raw` | Debugging HID mapping, deadzone, or device selection |
| `read_ft_sensor` | Serial wrench streaming with first-frame timeout |

Python mirrors live under `examples/python/` with the same CLI flags where applicable.

## Update Robot External Input

Use `teleop_spacemouse_sdk` or `fdcc_external_ft_sensor` to read a local
peripheral and call `robot.Peripheral().Update*Sample(...)`. The SDK owns the
internal DDS publisher; user code should not publish DDS messages directly.

`--robot <robot-ip>` is optional. If it is omitted, the example passes an empty
IP string to `smrcore_sdk`, which lets the SDK use its default connection
behavior.

Start the bridge before enabling Teleoperation or FDCC so rcore sees fresh
external input. See `docs/platform_setup.md` for permissions and robot reachability.

## F/T Calibration

`ft_sensor_calib_external` shows the intended composition point for external
F/T calibration: gather raw samples through the peripheral artifact, then call
the robot SDK calibration APIs (`PreviewFtCalibration` / `SaveFtCalibration`) from
your client after collecting static windows.
