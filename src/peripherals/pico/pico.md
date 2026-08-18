> SRC: pico.hpp, pico.cpp
> DIR: src/peripherals/pico/

# Pico

`Pico` 是不依赖机器人 SDK 的 WebXR 外设门面。

- `Initialize(PicoOptions)`: 保存 localhost 监听端口，默认 8765
- `Start()` / `Stop()`: 管理内置 HTTP/WebSocket 接收服务
- `GetSample()`: consume-on-read 获取最新 `PicoSample`
- `IsConnected()`: 表示 PICO WebXR WebSocket 客户端已连接
- `GetLastError()`: 返回最近一次启动失败原因

不负责机器人连接、坐标映射、Grip 离合或运动控制。

> FLOW: PICO WebXR → `protocols/pico/WebXrServer` → 最新样本槽 → `Pico::GetSample()`
