#!/usr/bin/env python3
"""Mirror of app/app_peripherals_read_spacemouse.cpp."""

from __future__ import annotations

import time

from rcore_peripherals import SpaceMouse, SpaceMouseOptions


def main() -> int:
    peripheral = SpaceMouse()
    options = SpaceMouseOptions(sample_rate_hz=125.0)
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
