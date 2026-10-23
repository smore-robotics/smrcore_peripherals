> DIR: unittests/

- test_pico_legacy_release.py: offline comparison-stop tests; 100 fixed-target
  frames, separate TCP/J6 paths, stale/fault/invalid feedback rejection.

# Overview

无硬件语义单测，由 `CMakeLists.txt` 中 `add_peripheral_test()` 注册 CTest。

# Contents

- ut_spacemouse_sample_model.cpp: 轴映射/符号/死区、夹爪锁存（含双键冲突保持）、定频 consume-on-read
- ut_kunwei_parser.cpp: 坤维 `KunweiParser` 帧解析与 `DrainLatest`
- ut_ft_sensor_options.cpp: `FtSensor::Initialize(FtSensorOptions)` 后 `GetInfo()` 字段（路径/型号/采样率）
- ut_xjc_parser.cpp: 实测 XJC 16 字节 int16 / 28 字节 float 帧解析，分片/重同步/CRC 拒绝，以及启停命令

- ut_pico_singularity_scale.cpp: 硬边界不受离开方向放宽；覆盖 30/45/60、非有限输入、整圈跳解、关节跨度和实测肘部危险姿态。

- `test_pico_stop_worker.py`: 编译生产停止目标/worker，验证有方向的目标估计、越限/无效参数拒绝、J6 比例限速、ServoJ 固定目标、延迟发送与停稳门禁；fake 不提供透传/StopMotion，并禁止遥操源码重新引入透传。

- test_zhonglin.py: mapping, invalid/stale samples, bridge rejection and cleanup without hardware.

- test_pico_python_launcher.py: no-hardware argument forwarding, exit status, missing binary and help tests.

- test_weekly_release.py: temporary Git repo checks for initial/patch tags, skip/retry and rollback refusal.

- test_release_notes.py: release identity, four-chain scope, invalid tags and SDK-doc consistency.
