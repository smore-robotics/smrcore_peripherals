> SRC: src/protocols/ft_sensor/xjc/xjc_parser.hpp, xjc_parser.cpp
> REF: serial_reader.hpp, peripherals/types.hpp

# XjcParser

鑫精诚 28 字节主动上报帧流式解析：`Feed()` / `DrainLatest()` / `Reset()` / `frame_error_count()`。

启停命令：`BuildStartCommand(hz)`（100/250/500/1000 Hz）、`BuildStopCommand()`。

> FLOW: 末字节 0x3C 对齐 28 字节帧 → 偏移 4 起 6×float32 LE（N / N·m）→ FtSensorSample
