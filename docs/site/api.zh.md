# API 参考

本页索引外设库的公开头文件及生命周期接口。完整接口签名与字段说明以链接的源码为准。

## C++ 公开接口

命名空间：`smrcore::peripherals`。

| 接口 | 参考 |
| --- | --- |
| Peripheral | [读取器基类](https://github.com/smore-robotics/smrcore_peripherals/blob/fa68bdc/src/peripherals/peripheral.hpp) |
| SpaceMouse | [SpaceMouse 读取器](https://github.com/smore-robotics/smrcore_peripherals/blob/fa68bdc/src/peripherals/spacemouse/spacemouse.hpp) |
| FtSensor | [六维力读取器](https://github.com/smore-robotics/smrcore_peripherals/blob/fa68bdc/src/peripherals/ft_sensor/ft_sensor.hpp) |
| Pico | [Pico WebXR 读取器](https://github.com/smore-robotics/smrcore_peripherals/blob/fa68bdc/src/peripherals/pico/pico.hpp) |
| 配置与采样类型 | [types.hpp](https://github.com/smore-robotics/smrcore_peripherals/blob/fa68bdc/src/peripherals/types.hpp) |

## 生命周期

具体设备通过 `Initialize(options)` 初始化，然后 `Start()` 启动后台读取。`Stop()` 停止读取但保留配置；`Shutdown()` 释放资源，之后需要重新初始化。

`IsConnected()` 查询连接状态，`GetInfo()` 获取低频诊断信息。Pico 的 `GetSample()` 非阻塞取走尚未消费的最新样本，无新样本时返回空值；`GetLastError()` 提供最近一次启动失败原因。

对象销毁不得与其他方法并发。

## Python

导入模块为 `rcore_peripherals`，使用示例见 [Python 示例](python.md)。机器人控制接口属于独立的 [Smartmore Robot SDK](https://smore-robotics.github.io/smrcore_sdk/zh/api/)，不属于基础外设 wheel。

此页是接口索引，尚未生成逐符号的 Doxygen API 页面。开发分支新增接口可能尚未合入 main，请对照所用版本查看头文件。
