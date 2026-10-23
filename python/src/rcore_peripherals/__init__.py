"""rcore_peripherals package bootstrap."""

from __future__ import annotations

import os
from pathlib import Path

_dll_dir = Path(__file__).resolve().parent / ".libs"

if _dll_dir.exists():
    if os.name == "nt":
        os.add_dll_directory(str(_dll_dir))
    else:
        existing = os.environ.get("LD_LIBRARY_PATH", "")
        parts = [p for p in existing.split(os.pathsep) if p]
        libdir = str(_dll_dir)
        if libdir not in parts:
            os.environ["LD_LIBRARY_PATH"] = (
                os.pathsep.join((libdir,) + tuple(parts)) if parts else libdir
            )

from ._version import __version__
from .data import (
    FtSensorOptions,
    FtSensorSample,
    GripperCommand,
    PeripheralInfo,
    PeripheralOptions,
    PicoControllerPose,
    PicoHeadsetPose,
    PicoOptions,
    PicoSample,
    SpaceMouseOptions,
    SpaceMouseRawSample,
    SpaceMouseSample,
)
from .ft_sensor import FtSensor
from .pico import Pico
from .spacemouse import SpaceMouse

__all__ = [
    "__version__",
    "FtSensor",
    "FtSensorOptions",
    "FtSensorSample",
    "GripperCommand",
    "PeripheralInfo",
    "PeripheralOptions",
    "Pico",
    "PicoControllerPose",
    "PicoHeadsetPose",
    "PicoOptions",
    "PicoSample",
    "SpaceMouse",
    "SpaceMouseOptions",
    "SpaceMouseRawSample",
    "SpaceMouseSample",
]

# Zhonglin is optional: importing other readers does not require numpy/pyserial.
def __getattr__(name):
    if name == "ZhonglinReader":
        from .zhonglin import ZhonglinReader
        return ZhonglinReader
    raise AttributeError(name)
