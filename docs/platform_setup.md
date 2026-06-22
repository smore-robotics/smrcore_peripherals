# Platform Setup

## SpaceMouse

On Linux, the user running the example must be able to read the selected
`/dev/input/event*` node. Use a udev rule or run under a group that has read
access to the device.

Optional `--device` selects a specific event node; omit it to use the default
discovery path. `--sample-rate` sets the reader poll rate in Hz (decoded read
defaults to 125 Hz in `read_spacemouse`).

## F/T Sensor

For serial F/T sensors, the user must have read/write access to the serial port,
for example `/dev/ttyUSB0`. Default examples use sensor type `xjc_serial` and
baud rate `460800` unless overridden on the CLI.

`probe` and `read_ft_sensor` accept `--serial-port`. Bridge and read examples
call `WaitForFirstSample()` and exit if the first valid frame times out.

## Robot Connection

Bridge examples (`teleop_spacemouse_sdk`, `fdcc_external_ft_sensor`,
`ft_sensor_calib_external`) require a reachable robot controller and a matching
`smrcore_sdk` artifact version. Start the bridge before enabling Teleoperation
or FDCC so rcore can see fresh external input.
