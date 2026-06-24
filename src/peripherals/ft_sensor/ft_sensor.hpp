/**
 * @file ft_sensor.hpp
 * @brief 六维力传感器外设门面
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include "peripherals/peripheral.hpp"

#include <memory>
#include <optional>

namespace smrcore::peripherals
{

/**
 * @class FtSensor
 * @brief 本地力传感器读取器与原始六维力采样生产者
 *
 * 基于坤维或鑫精诚（XJC）串口协议（见 `protocols/ft_sensor/kunwei/`、`xjc/`）
 * 以事件驱动方式读帧。
 * 可通过机器人 SDK `UpdateFtSensorSample()` 送入控制器。
 *
 * 线程安全：公共方法内部已同步；对象销毁不得与其他方法并发。
 */
class FtSensor : public Peripheral
{
public:
    FtSensor();
    ~FtSensor() override;

    FtSensor(const FtSensor &) = delete;
    FtSensor &operator=(const FtSensor &) = delete;

    /**
     * @brief 配置串口路径与波特率，不启动数据流
     *
     * @param options 使用 `serial_port`、`baud_rate`、`sample_rate_hz` 等字段
     * @return 选项是否被接受（当前实现恒为 true）
     */
    bool Initialize(const FtSensorOptions &options);

    /**
     * @brief 等待首帧有效采样
     *
     * 需先 `Initialize()` 和 `Start()`；超时时间为
     * `max(100ms, 5 * options.stale_timeout_ms)`。
     */
    bool WaitForFirstSample();

    /**
     * @brief 停止读取并释放串口与后台线程
     *
     * 可重复调用；调用后 `IsConnected()` 为 false。
     */
    void Shutdown() override;

    /**
     * @brief 打开串口并启动事件驱动读取循环
     *
     * @return `serial_port` 为空、串口打开失败或启流命令失败时为 false
     */
    bool Start() override;

    /**
     * @brief 停止读取循环，保留 `Initialize()` 配置
     */
    void Stop() override;

    /**
     * @brief 查询串口是否已打开且读取循环在运行
     */
    bool IsConnected() const override;

    /**
     * @brief 获取设备路径、型号与帧错误计数等诊断信息
     */
    PeripheralInfo GetInfo() const override;

    /**
     * @brief 非阻塞 consume-on-read 原始六维力采样
     *
     * 一次 drain 解析多帧时只保留最新帧；无新有效帧时返回 `std::nullopt`，
     * 不重复发送上一帧。返回值可直接用于 SDK `UpdateFtSensorSample()`。
     */
    std::optional<FtSensorSample> GetSample();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace smrcore::peripherals
