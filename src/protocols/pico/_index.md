> DIR: src/protocols/pico/

# Overview

PICO WebXR 的 localhost HTTP/WebSocket 接收协议层，仅由 `Pico` 门面组合。

# Contents

- webxr_server.hpp/cpp: 内嵌 WebXR 页面、完成 WebSocket 握手与帧读取，校验
  JSON 位姿/按键/双轴 thumbstick 数据并缓存尚未消费的最新 `PicoSample`；缺少 thumbstick 字段时兼容旧客户端并使用零值

> FLOW: `adb reverse` localhost 页面 → WebXR 50Hz JSON → 校验 → `PicoSample`
