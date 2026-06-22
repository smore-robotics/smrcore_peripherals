#!/usr/bin/env python3
"""Mirror of examples/cpp/probe.cpp."""

from __future__ import annotations

import argparse

from smrcore_peripherals import FtSensor, FtSensorOptions, SpaceMouse, SpaceMouseOptions


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Probe peripheral connectivity")
    parser.add_argument("--serial-port", default="/dev/ttyUSB0")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    spacemouse = SpaceMouse()
    sm_options = SpaceMouseOptions()
    spacemouse.Initialize(sm_options)
    sm_info = spacemouse.GetInfo()
    print(
        f"spacemouse model={sm_info.model} rate_hz={sm_info.sample_rate_hz} "
        f"connected={sm_info.connected}",
        flush=True,
    )

    ft_sensor = FtSensor()
    ft_options = FtSensorOptions(serial_port=args.serial_port)
    ft_sensor.Initialize(ft_options)
    ft_info = ft_sensor.GetInfo()
    print(
        f"ft_sensor model={ft_info.model} rate_hz={ft_info.sample_rate_hz} "
        f"connected={ft_info.connected}",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
