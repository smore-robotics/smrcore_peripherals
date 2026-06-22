#!/usr/bin/env python3
"""Skeleton: SpaceMouse samples -> robot external input (see C++ teleop_spacemouse_sdk).

The released smrcore_sdk Python wheel may not yet expose Peripheral().UpdateSpaceMouseSample().
Use examples/cpp/teleop_spacemouse_sdk for the full bridge workflow.
"""

from __future__ import annotations


def main() -> int:
    raise SystemExit(
        "Install matching smrcore_sdk and smrcore_peripherals wheels, then mirror "
        "examples/cpp/teleop_spacemouse_sdk.cpp: read SpaceMouse samples and call "
        "robot.Peripheral().UpdateSpaceMouseSample() when the Python SDK exposes it."
    )


if __name__ == "__main__":
    raise SystemExit(main())
