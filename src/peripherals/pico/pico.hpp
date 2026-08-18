/**
 * @file pico.hpp
 * @brief PICO WebXR 外设门面
 */

#pragma once

#include "peripherals/peripheral.hpp"

#include <memory>
#include <optional>
#include <string>

namespace smrcore::peripherals
{

/**
 * @brief 在本机接收 PICO 头显和双手柄位姿的独立外设
 *
 * 本类只管理 WebXR HTTP/WebSocket 接收与最新样本缓存，不依赖机器人 SDK，
 * 不接受机器人地址，也不执行坐标映射或运动控制。
 */
class Pico : public Peripheral
{
public:
    Pico();
    ~Pico() override;

    Pico(const Pico &) = delete;
    Pico &operator=(const Pico &) = delete;

    bool Initialize(const PicoOptions &options);
    void Shutdown() override;
    bool Start() override;
    void Stop() override;
    bool IsConnected() const override;
    PeripheralInfo GetInfo() const override;

    /** @brief 非阻塞取走尚未消费的最新 WebXR 采样。 */
    std::optional<PicoSample> GetSample();

    /** @brief 返回最近一次 Start() 失败原因。 */
    std::string GetLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace smrcore::peripherals
