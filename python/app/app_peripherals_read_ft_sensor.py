#!/usr/bin/env python3
"""Mirror of app/app_peripherals_read_ft_sensor.cpp."""

from __future__ import annotations

import argparse
import time

from rcore_peripherals import FtSensor, FtSensorOptions


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Read F/T sensor samples")
    parser.add_argument("--serial-port", default="/dev/ttyUSB0")
    parser.add_argument("--sensor-type", default="xjc_serial")
    parser.add_argument("--baud-rate", type=int, default=460800)
    parser.add_argument("--sample-rate", type=float, default=1000.0)
    parser.add_argument("--read-buffer-size", type=int, default=512)
    parser.add_argument("--stale-timeout-ms", type=int, default=20)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    peripheral = FtSensor()
    options = FtSensorOptions(
        serial_port=args.serial_port,
        sensor_type=args.sensor_type,
        baud_rate=args.baud_rate,
        sample_rate_hz=args.sample_rate,
        read_buffer_size=args.read_buffer_size,
        stale_timeout_ms=args.stale_timeout_ms,
    )
    if not peripheral.Initialize(options):
        print("failed to initialize F/T peripheral", flush=True)
        return 1

    print(
        f"serial_port={options.serial_port} baud_rate={options.baud_rate} "
        f"type={options.sensor_type}",
        flush=True,
    )

    if not peripheral.Start():
        print("failed to start F/T peripheral", flush=True)
        return 1

    if not peripheral.WaitForFirstSample():
        info = peripheral.GetInfo()
        print(
            f"first valid frame timeout, frame_errors={info.frame_error_count}",
            flush=True,
        )
        return 1

    while True:
        sample = peripheral.GetSample()
        if sample is not None:
            print(
                sample.fx,
                sample.fy,
                sample.fz,
                sample.tx,
                sample.ty,
                sample.tz,
                flush=True,
            )
        time.sleep(0.001)


if __name__ == "__main__":
    raise SystemExit(main())
