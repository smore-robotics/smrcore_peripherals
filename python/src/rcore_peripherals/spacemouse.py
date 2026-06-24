from __future__ import annotations

from typing import Optional

from . import _native
from .data import PeripheralInfo, SpaceMouseOptions, SpaceMouseRawSample, SpaceMouseSample


class SpaceMouse:
    def __init__(self, _native_mouse=None):
        self._native = _native_mouse if _native_mouse is not None else _native.SpaceMouse()

    def Initialize(self, options: SpaceMouseOptions | dict) -> bool:
        if isinstance(options, SpaceMouseOptions):
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

    def GetSample(self) -> Optional[SpaceMouseSample]:
        sample = self._native.GetSample()
        if sample is None:
            return None
        return SpaceMouseSample.from_dict(sample)

    def GetRawSample(self) -> Optional[SpaceMouseRawSample]:
        sample = self._native.GetRawSample()
        if sample is None:
            return None
        return SpaceMouseRawSample.from_dict(sample)
