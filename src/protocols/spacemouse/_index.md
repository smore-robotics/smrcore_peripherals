> DIR: src/protocols/spacemouse/

# Overview

SpaceMouse / 3Dconnexion Linux input 子系统读取，输出未归一化的 6 轴 + 按键帧。

# Contents

- linux_input_reader.hpp/cpp: `LinuxInputReader`、`DeviceInfo`、`RawInputFrame`
  - `ScanDevices()`: 遍历 `/dev/input/event*`，按设备名或 3Dconnexion USB vendor 过滤
  - `Open(event_path)` / `ReadFrame(frame, timeout_ms)`: poll 读 EV_ABS/EV_KEY，类内累积绝对轴与按键位掩码

> FLOW: evdev 事件 → 累积 `axes[6]`/`buttons` → `RawInputFrame` → 门面 `SampleModel` 定频归一化
