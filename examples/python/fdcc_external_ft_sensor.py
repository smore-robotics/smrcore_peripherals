#!/usr/bin/env python3
"""Skeleton: external F/T samples -> robot FDCC input (see C++ fdcc_external_ft_sensor).

The released smrcore_sdk Python wheel may not yet expose Peripheral().UpdateFtSensorSample().
Use examples/cpp/fdcc_external_ft_sensor for the full bridge workflow.
"""

from __future__ import annotations


def main() -> int:
    raise SystemExit(
        "Install matching smrcore_sdk and smrcore_peripherals wheels, then mirror "
        "examples/cpp/fdcc_external_ft_sensor.cpp: stream F/T samples and call "
        "robot.Peripheral().UpdateFtSensorSample() when the Python SDK exposes it."
    )


if __name__ == "__main__":
    raise SystemExit(main())
