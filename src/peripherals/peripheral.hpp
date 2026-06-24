/**
 * @file peripheral.hpp
 * @brief 外设抽象基类
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include "peripherals/types.hpp"

namespace smrcore::peripherals
{

/**
 * @class Peripheral
 * @brief 本地外设读取器抽象基类
 *
 * `FtSensor`、`SpaceMouse` 等具体外设派生本类，负责线程与资源管理；
 * 底层协议解析与设备 IO 位于 `src/protocols/`。
 *
 * 线程安全：公共方法内部已同步；对象销毁不得与其他方法并发。
 */
class Peripheral
{
public:
    virtual ~Peripheral() = default;

    /**
     * @brief 停止采样并释放本地资源
     *
     * 可重复调用。调用后需重新 `Initialize()` 才能 `Start()`。
     */
    virtual void Shutdown() = 0;

    /**
     * @brief 启动后台读取循环
     *
     * @return 未 `Initialize()`、设备路径/串口无效或打开失败时为 false
     */
    virtual bool Start() = 0;

    /**
     * @brief 停止读取循环，保留 `Initialize()` 时的配置
     *
     * 可重复调用，用于暂停采样而不丢弃选项。
     */
    virtual void Stop() = 0;

    /**
     * @brief 查询管理面连接状态
     *
     * @return 本地读取器是否认为设备当前已连接并可产生采样
     */
    virtual bool IsConnected() const = 0;

    /**
     * @brief 获取低频设备信息与诊断快照
     *
     * 不含高频采样数据，供 UI、日志与探测工具使用。
     */
    virtual PeripheralInfo GetInfo() const = 0;

protected:
    Peripheral() = default;
    Peripheral(const Peripheral &) = delete;
    Peripheral &operator=(const Peripheral &) = delete;
};

} // namespace smrcore::peripherals
