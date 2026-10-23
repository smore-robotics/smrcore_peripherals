# Peripheral Readers

Run these commands from the repository root after installing the reader wheel.

## SpaceMouse

```bash
./build_Release/bin/app_peripherals_probe
python python/app/app_peripherals_read_spacemouse.py --device /dev/input/event5 --sample-rate 125
```

Replace the example device node with the detected one.

## Force/torque sensor

```bash
python python/app/app_peripherals_read_ft_sensor.py --serial-port /dev/ttyUSB0 --sensor-type xjc_serial --baud-rate 460800
```

Select the protocol and baud rate matching your sensor. See [platform permissions](https://github.com/smore-robotics/smrcore_peripherals/blob/main/docs/platform_setup.en.md).

## Pico

```bash
python python/app/app_peripherals_read_pico.py --port 8765
```

With USB debugging enabled and authorized, run in another terminal:

```bash
adb devices
adb reverse tcp:8765 tcp:8765
```

Open `http://localhost:8765/` in the headset browser and enter VR. Positions are WebXR reference-space XYZ in meters; untracked poses are N/A.

!!! note "Wi-Fi"
    localhost works here through adb reverse. For ordinary Wi-Fi, localhost means the headset itself. WebXR also requires a secure context: use a headset-accessible HTTPS endpoint with a trusted certificate rather than simply replacing localhost with an HTTP PC address.
