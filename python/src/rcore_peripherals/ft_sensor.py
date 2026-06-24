from __future__ import annotations

from typing import Optional

from . import _native
from .data import FtSensorOptions, FtSensorSample, PeripheralInfo


class FtSensor:
    def __init__(self, _native_sensor=None):
        self._native = _native_sensor if _native_sensor is not None else _native.FtSensor()

    def Initialize(self, options: FtSensorOptions | dict) -> bool:
        if isinstance(options, FtSensorOptions):
            return bool(self._native.Initialize(options.to_native()))
        return bool(self._native.Initialize(options))

    def Shutdown(self) -> None:
        self._native.Shutdown()

    def Start(self) -> bool:
        return bool(self._native.Start())

    def Stop(self) -> None:
        self._native.Stop()

    def IsConnected(self) -> bool:
        return bool(self._native.IsConnected())

    def GetInfo(self) -> PeripheralInfo:
        return PeripheralInfo.from_dict(self._native.GetInfo())

    def WaitForFirstSample(self) -> bool:
        return bool(self._native.WaitForFirstSample())

    def GetSample(self) -> Optional[FtSensorSample]:
        sample = self._native.GetSample()
        if sample is None:
            return None
        return FtSensorSample.from_dict(sample)
