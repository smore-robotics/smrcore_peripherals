/**
 * @file spacemouse.hpp
 * @brief SpaceMouse 外设门面
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
 * @class SpaceMouse
 * @brief SpaceMouse 本地读取器与归一化采样生产者
 *
 * 通过 Linux input 子系统读取原始事件（见 `protocols/spacemouse/`），
 * 以固定频率输出归一化状态。采样语义与 rcore SDK
 * `UpdateSpaceMouseSample()` 输入的 `SpaceMouseSample` 一致。
 *
 * 线程安全：公共方法内部已同步；对象销毁不得与其他方法并发。
 */
class SpaceMouse : public Peripheral
{
public:
    SpaceMouse();
    ~SpaceMouse() override;

    SpaceMouse(const SpaceMouse &) = delete;
    SpaceMouse &operator=(const SpaceMouse &) = delete;

    /**
     * @brief 配置设备路径、轴映射与固定输出频率，不启动采样
     *
     * @param options 使用 `device_path`、轴映射/死区及 `sample_rate_hz` 字段；
     *                `device_path` 为空时仅探测并填充 `GetInfo().path`
     * @return 选项是否被接受（当前实现恒为 true）
     */
    bool Initialize(const SpaceMouseOptions &options);

    /**
     * @brief 停止采样并释放 input 设备与后台线程
     *
     * 可重复调用；调用后 `IsConnected()` 为 false。
     */
    void Shutdown() override;

    /**
     * @brief 打开 input 设备并启动定频采样循环
     *
     * @return 未找到设备或 `open()` 失败时为 false
     */
    bool Start() override;

    /**
     * @brief 停止采样循环，保留 `Initialize()` 配置
     */
    void Stop() override;

    /**
     * @brief 查询 input 设备是否已打开且采样循环在运行
     */
    bool IsConnected() const override;

    /**
     * @brief 获取设备路径、型号与有效采样率等诊断信息
     */
    PeripheralInfo GetInfo() const override;

    /**
     * @brief 非阻塞 consume-on-read 归一化采样
     *
     * 返回后槽位清空；下一 tick 即使无新 Linux 事件也会发布最新维护状态。
     * 返回值可直接用于 SDK `UpdateSpaceMouseSample()`。
     */
    std::optional<SpaceMouseSample> GetSample();

    /**
     * @brief 非阻塞 consume-on-read 原始轴/按键采样
     *
     * 与 `GetSample()` 共享定频 tick，但不经归一化与死区处理。
     */
    std::optional<SpaceMouseRawSample> GetRawSample();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace smrcore::peripherals
