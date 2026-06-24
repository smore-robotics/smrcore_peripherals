> DIR: src/protocols/

# Overview

协议与底层 IO 层，不含采样线程与归一化逻辑。由 `peripherals/` 门面组合调用。

# Contents

- ft_sensor/: 力传感器串口传输与品牌协议，见 `ft_sensor/_index.md`
- spacemouse/: Linux evdev 读取，见 `spacemouse/_index.md`

> FLOW: 字节/事件 → Parser/Reader → `FtSensorSample` 或 `RawInputFrame`；门面层再做线程与槽管理
