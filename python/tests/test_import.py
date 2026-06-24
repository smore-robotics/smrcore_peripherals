from __future__ import annotations

import re


def test_package_imports():
    import rcore_peripherals

    assert rcore_peripherals.__version__


def test_native_extension_imports():
    from rcore_peripherals import _native

    assert hasattr(_native, "build_info")
    assert _native.build_info()["backend"] == "pybind11"
    assert _native.linked_sdk()["linked"] is True


def test_linked_sdk_reports_cpp_version():
    from rcore_peripherals import _native

    info = _native.linked_sdk()
    assert "version" in info
    assert re.fullmatch(r"\d+\.\d+\.\d+", info["version"]), info["version"]


def test_native_types_are_registered():
    from rcore_peripherals import _native

    ft_sensor = _native.FtSensor()
    spacemouse = _native.SpaceMouse()
    assert hasattr(ft_sensor, "Initialize")
    assert hasattr(spacemouse, "GetRawSample")
