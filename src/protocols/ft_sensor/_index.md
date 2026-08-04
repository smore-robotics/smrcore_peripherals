> DIR: src/protocols/ft_sensor/

# Overview

力传感器串口栈：POSIX 传输 → 品牌无关批量读 → 品牌 Parser/Reader 启停流。

# Contents

- port/posix_serial_port.hpp: `port::PosixSerialPort` termios 8N1 非阻塞 open/read/write
- serial_reader.hpp/cpp: `SerialReader`、`SerialReaderConfig`；模板 `ReadAvailable(Parser&)` 读字节→`Feed`→样本向量
- kunwei/kunwei_parser.hpp/cpp: `KunweiParser` 28 字节坤维帧，见 `kunwei/kunwei_parser.md`
- kunwei/kunwei_reader.hpp/cpp: `KunweiReader`、`KunweiReaderConfig`（= `SerialReaderConfig`）；启停流 `0x48aa`/`0x43aa`
- xjc/xjc_parser.hpp/cpp: `XjcParser` 按频率解析 16 字节 int16 / 28 字节 float XJC 主动上报帧，校验 Modbus CRC，并生成启停命令；见 `xjc/xjc_parser.md`
- xjc/xjc_reader.hpp/cpp: `XjcReader`、`XjcReaderConfig`（串口配置 + `active_reporting_hz` 100/250/500/1000）

> FLOW: Reader::Open → StartStreaming → SerialReader::ReadAvailable(parser) → Parser::Feed → FtSensorSample
