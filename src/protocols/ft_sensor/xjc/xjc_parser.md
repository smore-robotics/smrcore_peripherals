> SRC: src/protocols/ft_sensor/xjc/xjc_parser.hpp, xjc_parser.cpp
> REF: serial_reader.hpp, peripherals/types.hpp

# XjcParser

鑫精诚主动上报帧流式解析：`SetReportingHz()` 选择格式，`Feed()` / `DrainLatest()` / `Reset()` / `frame_error_count()` 处理分片与重同步。

- 100/250/500 Hz：16 字节，`0x20 0x4e` + 6×int16 LE + Modbus CRC16；力缩放 1/100，力矩缩放 1/1000
- 1000 Hz：28 字节，`0x20 0x4e` + 6×float32 BE + Modbus CRC16

启停命令：`BuildStartCommand(hz)`（100/250/500/1000 Hz）、`BuildStopCommand()`。

> FLOW: 搜索 `0x20 0x4e` 帧头 → 按上报频率确定 16/28 字节帧 → 验证 CRC → 解码为 FtSensorSample
