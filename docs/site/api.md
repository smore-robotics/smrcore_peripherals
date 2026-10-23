# API Reference

This page indexes public peripheral headers and lifecycle methods. Follow the source links for signatures and field documentation.

## C++ interfaces

Namespace: `smrcore::peripherals`.

| Interface | Header |
| --- | --- |
| Peripheral | [Base reader](https://github.com/zhanhui-li-smore/smrcore_peripherals/blob/fa68bdc/src/peripherals/peripheral.hpp) |
| SpaceMouse | [SpaceMouse reader](https://github.com/zhanhui-li-smore/smrcore_peripherals/blob/fa68bdc/src/peripherals/spacemouse/spacemouse.hpp) |
| FtSensor | [Force/torque reader](https://github.com/zhanhui-li-smore/smrcore_peripherals/blob/fa68bdc/src/peripherals/ft_sensor/ft_sensor.hpp) |
| Pico | [WebXR reader](https://github.com/zhanhui-li-smore/smrcore_peripherals/blob/fa68bdc/src/peripherals/pico/pico.hpp) |
| Options and samples | [types.hpp](https://github.com/zhanhui-li-smore/smrcore_peripherals/blob/fa68bdc/src/peripherals/types.hpp) |

## Lifecycle

Initialize a concrete reader with `Initialize(options)`, then call `Start()`. `Stop()` stops reading while retaining configuration; `Shutdown()` releases resources and requires reinitialization before restarting.

`IsConnected()` reports connection state and `GetInfo()` returns diagnostic information. Pico `GetSample()` consumes the latest unread sample without blocking and returns an empty optional if none is available. `GetLastError()` reports the most recent start failure. Do not destroy an object concurrently with other methods.

## Python

Import `rcore_peripherals`; see [Python examples](python.md). Robot control belongs to the separate [Smartmore Robot SDK](https://smore-robotics.github.io/smrcore_sdk/api/), not the reader wheel.

This is an interface index, not generated per-symbol Doxygen documentation. Development interfaces may not yet be merged into main; consult headers from your actual version.
