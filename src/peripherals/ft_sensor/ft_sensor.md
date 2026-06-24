> SRC: ft_sensor.hpp, ft_sensor.cpp
> DIR: src/peripherals/ft_sensor/
> REF: src/peripherals/types.hpp
> FLOW: FtSensor::Initialize(FtSensorOptions) -> Start -> WaitForFirstSample -> GetSample

# FtSensor

- `FtSensor`: 坤维/XJC 串口六维力读取门面；`Start()` 发送启流命令，后台线程 drain 解析，只保留最新帧
- `Initialize(options)`: 使用 `FtSensorOptions` 配置串口、协议类型、波特率、采样率和首帧等待参数
- `WaitForFirstSample()`: 首帧等待，超时 `max(100ms, 5*options.stale_timeout_ms)`，需先完成 `Initialize()` 和 `Start()`
