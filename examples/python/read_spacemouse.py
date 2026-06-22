#!/usr/bin/env python3
"""Mirror of examples/cpp/read_spacemouse.cpp."""

from __future__ import annotations

import argparse
import time

from smrcore_peripherals import SpaceMouse, SpaceMouseOptions


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Read SpaceMouse samples")
    parser.add_argument("--device", default="", help="Optional /dev/input/event* path")
    parser.add_argument("--sample-rate", type=float, default=125.0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    peripheral = SpaceMouse()
    options = SpaceMouseOptions(sample_rate_hz=args.sample_rate)
    if args.device:
        options.device_path = args.device
    if not peripheral.Initialize(options) or not peripheral.Start():
        print("failed to start SpaceMouse peripheral", flush=True)
        return 1

    while True:
        sample = peripheral.GetSample()
        if sample is not None:
            print(
                sample.x,
                sample.y,
                sample.z,
                sample.roll,
                sample.pitch,
                sample.yaw,
                flush=True,
            )
        time.sleep(0.002)


if __name__ == "__main__":
    raise SystemExit(main())
