from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, Iterator, Sequence

from . import _native

GripperCommand = _native.GripperCommand
GripperOpen = GripperCommand.Open
GripperClose = GripperCommand.Close


def _six_ints(values: Iterable[int], name: str) -> tuple[int, ...]:
    normalized = tuple(int(value) for value in values)
    if len(normalized) != 6:
        raise ValueError(f"{name} must contain exactly 6 values")
    return normalized


def _six_floats(values: Iterable[float], name: str) -> tuple[float, ...]:
    normalized = tuple(float(value) for value in values)
    if len(normalized) != 6:
        raise ValueError(f"{name} must contain exactly 6 values")
    return normalized


@dataclass(frozen=True)
class PeripheralOptions:
    sample_rate_hz: float = 0.0

    def __init__(self, sample_rate_hz: float = 0.0):
        object.__setattr__(self, "sample_rate_hz", float(sample_rate_hz))

    def to_native(self) -> dict:
        return self.to_dict()

    def to_dict(self) -> dict:
        return {"sample_rate_hz": self.sample_rate_hz}


@dataclass(frozen=True)
class FtSensorOptions(PeripheralOptions):
    sample_rate_hz: float = 0.0
    serial_port: str = ""
    baud_rate: int = 460800
    read_buffer_size: int = 512
    sensor_type: str = "kunwei_serial"
    stale_timeout_ms: int = 20

    def __init__(
        self,
        sample_rate_hz: float = 0.0,
        serial_port: str = "",
        baud_rate: int = 460800,
        read_buffer_size: int = 512,
        sensor_type: str = "kunwei_serial",
        stale_timeout_ms: int = 20,
    ):
        super().__init__(sample_rate_hz=sample_rate_hz)
        object.__setattr__(self, "serial_port", str(serial_port))
        object.__setattr__(self, "baud_rate", int(baud_rate))
        object.__setattr__(self, "read_buffer_size", int(read_buffer_size))
        object.__setattr__(self, "sensor_type", str(sensor_type))
        object.__setattr__(self, "stale_timeout_ms", int(stale_timeout_ms))

    def to_dict(self) -> dict:
        data = super().to_dict()
        data.update(
            {
                "serial_port": self.serial_port,
                "baud_rate": self.baud_rate,
                "read_buffer_size": self.read_buffer_size,
                "sensor_type": self.sensor_type,
                "stale_timeout_ms": self.stale_timeout_ms,
            }
        )
        return data


@dataclass(frozen=True)
class SpaceMouseOptions(PeripheralOptions):
    sample_rate_hz: float = 0.0
    device_path: str = ""
    axis_map: tuple[int, ...] = (0, 1, 2, 3, 4, 5)
    axis_sign: tuple[int, ...] = (1, -1, -1, 1, -1, -1)
    axis_scale: tuple[float, ...] = (350.0, 350.0, 350.0, 350.0, 350.0, 350.0)
    deadzone: float = 0.01

    def __init__(
        self,
        sample_rate_hz: float = 0.0,
        device_path: str = "",
        axis_map: Iterable[int] | None = None,
        axis_sign: Iterable[int] | None = None,
        axis_scale: Iterable[float] | None = None,
        deadzone: float = 0.01,
    ):
        super().__init__(sample_rate_hz=sample_rate_hz)
        object.__setattr__(self, "device_path", str(device_path))
        object.__setattr__(
            self,
            "axis_map",
            _six_ints(axis_map if axis_map is not None else (0, 1, 2, 3, 4, 5), "axis_map"),
        )
        object.__setattr__(
            self,
            "axis_sign",
            _six_ints(
                axis_sign if axis_sign is not None else (1, -1, -1, 1, -1, -1),
                "axis_sign",
            ),
        )
        object.__setattr__(
            self,
            "axis_scale",
            _six_floats(
                axis_scale if axis_scale is not None else (350.0, 350.0, 350.0, 350.0, 350.0, 350.0),
                "axis_scale",
            ),
        )
        object.__setattr__(self, "deadzone", float(deadzone))

    def to_native(self) -> dict:
        return self.to_dict()

    def to_dict(self) -> dict:
        data = super().to_dict()
        data.update(
            {
                "device_path": self.device_path,
                "axis_map": list(self.axis_map),
                "axis_sign": list(self.axis_sign),
                "axis_scale": list(self.axis_scale),
                "deadzone": self.deadzone,
            }
        )
        return data


@dataclass(frozen=True)
class PeripheralInfo:
    path: str = ""
    model: str = ""
    serial_number: str = ""
    firmware_version: str = ""
    sample_rate_hz: float = 0.0
    connected: bool = False
    frame_error_count: int = 0

    @classmethod
    def from_dict(cls, data: dict) -> "PeripheralInfo":
        return cls(
            path=str(data.get("path", "")),
            model=str(data.get("model", "")),
            serial_number=str(data.get("serial_number", "")),
            firmware_version=str(data.get("firmware_version", "")),
            sample_rate_hz=float(data.get("sample_rate_hz", 0.0)),
            connected=bool(data.get("connected", False)),
            frame_error_count=int(data.get("frame_error_count", 0)),
        )


@dataclass(frozen=True)
class SpaceMouseSample:
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0
    roll: float = 0.0
    pitch: float = 0.0
    yaw: float = 0.0
    gripper_command: int = GripperOpen
    timestamp_sec: float = 0.0

    @classmethod
    def from_dict(cls, data: dict) -> "SpaceMouseSample":
        return cls(
            x=float(data.get("x", 0.0)),
            y=float(data.get("y", 0.0)),
            z=float(data.get("z", 0.0)),
            roll=float(data.get("roll", 0.0)),
            pitch=float(data.get("pitch", 0.0)),
            yaw=float(data.get("yaw", 0.0)),
            gripper_command=int(data.get("gripper_command", GripperOpen)),
            timestamp_sec=float(data.get("timestamp_sec", 0.0)),
        )


@dataclass(frozen=True)
class SpaceMouseRawSample:
    axes: tuple[int, ...]
    buttons: int = 0
    timestamp_sec: float = 0.0

    def __init__(
        self,
        axes: Iterable[int],
        buttons: int = 0,
        timestamp_sec: float = 0.0,
    ):
        object.__setattr__(self, "axes", _six_ints(axes, "axes"))
        object.__setattr__(self, "buttons", int(buttons))
        object.__setattr__(self, "timestamp_sec", float(timestamp_sec))

    def __iter__(self) -> Iterator[int]:
        return iter(self.axes)

    @classmethod
    def from_dict(cls, data: dict) -> "SpaceMouseRawSample":
        return cls(
            axes=data.get("axes", [0, 0, 0, 0, 0, 0]),
            buttons=int(data.get("buttons", 0)),
            timestamp_sec=float(data.get("timestamp_sec", 0.0)),
        )


@dataclass(frozen=True)
class FtSensorSample:
    fx: float = 0.0
    fy: float = 0.0
    fz: float = 0.0
    tx: float = 0.0
    ty: float = 0.0
    tz: float = 0.0
    timestamp_sec: float = 0.0

    def as_tuple(self) -> tuple[float, float, float, float, float, float]:
        return (self.fx, self.fy, self.fz, self.tx, self.ty, self.tz)

    @classmethod
    def from_dict(cls, data: dict) -> "FtSensorSample":
        return cls(
            fx=float(data.get("fx", 0.0)),
            fy=float(data.get("fy", 0.0)),
            fz=float(data.get("fz", 0.0)),
            tx=float(data.get("tx", 0.0)),
            ty=float(data.get("ty", 0.0)),
            tz=float(data.get("tz", 0.0)),
            timestamp_sec=float(data.get("timestamp_sec", 0.0)),
        )
