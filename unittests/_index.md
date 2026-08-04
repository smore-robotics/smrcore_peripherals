> DIR: unittests/

# Overview

无硬件语义单测，由 `CMakeLists.txt` 中 `add_peripheral_test()` 注册 CTest。

# Contents

- ut_spacemouse_sample_model.cpp: 轴映射/符号/死区、夹爪锁存（含双键冲突保持）、定频 consume-on-read
- ut_kunwei_parser.cpp: 坤维 `KunweiParser` 帧解析与 `DrainLatest`
- ut_ft_sensor_options.cpp: `FtSensor::Initialize(FtSensorOptions)` 后 `GetInfo()` 字段（路径/型号/采样率）
- ut_xjc_parser.cpp: 实测 XJC 16 字节 int16 / 28 字节 float 帧解析，分片/重同步/CRC 拒绝，以及启停命令
