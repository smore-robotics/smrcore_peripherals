from __future__ import annotations

from typing import Optional

from . import _native
from .data import PeripheralInfo, PicoOptions, PicoSample


class Pico:
    """PICO WebXR receiver with a Python-owned sampling loop."""

    def __init__(self, _native_pico=None):
        self._native = _native_pico if _native_pico is not None else _native.Pico()

    def Initialize(self, options: PicoOptions | dict) -> bool:
        if isinstance(options, PicoOptions):
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

    def GetSample(self) -> Optional[PicoSample]:
        sample = self._native.GetSample()
        if sample is None:
            return None
        return PicoSample.from_dict(sample)

    def GetLastError(self) -> str:
        return str(self._native.GetLastError())
