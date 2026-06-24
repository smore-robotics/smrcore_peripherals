> DIR: src/peripherals/

# Overview

对外公开 API 命名空间 `smrcore::peripherals`。聚合头 `peripherals.hpp` 一次引入全部类型与门面类。

# Contents

- types.hpp: 公共 DTO 与 `*Options` 配置结构体，见 `types.md`
- peripheral.hpp: 抽象基类 `Peripheral`（`Shutdown`/`Start`/`Stop`/`IsConnected`/`GetInfo`）
- peripherals.hpp: 聚合 `#include`（types + peripheral + ft_sensor + spacemouse）
- ft_sensor/ft_sensor.hpp: `FtSensor` 六维力传感器门面，见 `ft_sensor/ft_sensor.md`
- spacemouse/spacemouse.hpp: `SpaceMouse` 3D 鼠标门面
  - sample_model.hpp/cpp: 内部定频 tick、轴映射/死区/夹爪锁存与 `LatestSlot` 发布（不对外暴露）；双键同时按下视为冲突并保持上一锁存值

> FLOW: `Initialize(*Options)` 拷贝配置 → `Start()` 开设备+后台循环 → `GetSample()`/`GetRawSample()` consume-on-read
