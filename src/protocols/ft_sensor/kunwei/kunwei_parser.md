> SRC: src/protocols/ft_sensor/kunwei/kunwei_parser.hpp, kunwei_parser.cpp
> REF: serial_reader.hpp, peripherals/types.hpp

# KunweiParser

坤维 28 字节固定帧流式解析：`Feed()` / `DrainLatest()` / `Reset()` / `frame_error_count()`。

> FLOW: 帧头 0x48aa → 6×float32 LE × g → FtSensorSample
