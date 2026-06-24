> DIR: unittests/

# Overview

无硬件语义单测，由 `CMakeLists.txt` 中 `add_peripheral_test()` 注册 CTest。

# Contents

- ut_spacemouse_sample_model.cpp: 轴映射/符号/死区、夹爪锁存（含双键冲突保持）、定频 consume-on-read
- ut_kunwei_parser.cpp: 坤维 `KunweiParser` 帧解析与 `DrainLatest`
- ut_ft_sensor_options.cpp: `FtSensor::Initialize(FtSensorOptions)` 后 `GetInfo()` 字段（路径/型号/采样率）
- ut_xjc_parser.cpp: XJC 28 字节帧解析与 `BuildStartCommand` 数据/CRC 字节、`BuildStopCommand`
