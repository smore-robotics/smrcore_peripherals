from __future__ import annotations

from rcore_peripherals.data import (
    FtSensorOptions,
    FtSensorSample,
    PeripheralInfo,
    PeripheralOptions,
    SpaceMouseOptions,
    SpaceMouseRawSample,
    SpaceMouseSample,
)


def test_options_round_trip_dict():
    options = PeripheralOptions(
        sample_rate_hz=125.0,
    )
    assert options.to_dict() == {"sample_rate_hz": 125.0}

    spacemouse_options = SpaceMouseOptions(
        sample_rate_hz=125.0,
        device_path="/dev/input/event7",
    )
    data = spacemouse_options.to_dict()
    assert data["sample_rate_hz"] == 125.0
    assert data["device_path"] == "/dev/input/event7"
    assert len(data["axis_map"]) == 6

    ft_options = FtSensorOptions(
        sample_rate_hz=1000.0,
        serial_port="/dev/ttyUSB0",
        baud_rate=460800,
        read_buffer_size=1024,
        sensor_type="xjc_serial",
        stale_timeout_ms=30,
    )
    data = ft_options.to_dict()
    assert data["sample_rate_hz"] == 1000.0
    assert data["serial_port"] == "/dev/ttyUSB0"
    assert data["read_buffer_size"] == 1024
    assert data["sensor_type"] == "xjc_serial"
    assert data["stale_timeout_ms"] == 30


def test_sample_from_dict():
    sample = FtSensorSample.from_dict(
        {"fx": 1.0, "fy": 2.0, "fz": 3.0, "tx": 0.1, "ty": 0.2, "tz": 0.3}
    )
    assert sample.as_tuple() == (1.0, 2.0, 3.0, 0.1, 0.2, 0.3)

    sm = SpaceMouseSample.from_dict({"x": 0.5, "y": -0.2, "gripper_command": 1})
    assert sm.x == 0.5
    assert sm.gripper_command == 1

    raw = SpaceMouseRawSample.from_dict({"axes": [1, 2, 3, 4, 5, 6], "buttons": 3})
    assert list(raw.axes) == [1, 2, 3, 4, 5, 6]
    assert raw.buttons == 3

    info = PeripheralInfo.from_dict({"path": "/dev/ttyUSB0", "connected": True})
    assert info.path == "/dev/ttyUSB0"
    assert info.connected is True
