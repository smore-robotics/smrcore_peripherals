/**
 * @file xjc_parser.hpp
 * @brief 鑫精诚（XJC）六维力传感器主动上报模式帧解析
 *
 * XJC 主动上报帧以 `0x20 0x4e` 开头并以 Modbus CRC16 结束：
 * - 100/250/500 Hz：6× int16 LE，力除以 100，力矩除以 1000，共 16 字节
 * - 1000 Hz：6× float32 BE，单位 N / N·m，共 28 字节
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include "peripherals/types.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace smrcore::peripherals::protocols::ft_sensor::xjc
{

/**
 * @class XjcParser
 * @brief XJC 主动上报流式解析器
 */
class XjcParser
{
public:
    static constexpr std::size_t kIntegerFrameSize = 16;
    static constexpr std::size_t kFloatFrameSize = 28;
    static constexpr std::size_t kPayloadOffset = 2;

    /** @brief 选择与启流命令一致的主动上报帧格式 */
    bool SetReportingHz(int reporting_hz);

    std::vector<FtSensorSample> Feed(const uint8_t *data, std::size_t size,
                                     double timestamp_sec);

    std::optional<FtSensorSample>
    DrainLatest(const uint8_t *data, std::size_t size, double timestamp_sec);

    uint32_t frame_error_count() const { return frame_error_count_; }

    void Reset();

    /** @brief 构建主动上报启流 Modbus 命令（写保持寄存器 0x019A） */
    static std::vector<uint8_t> BuildStartCommand(int reporting_hz);

    /** @brief 构建停流命令（12 字节 0xFF） */
    static std::vector<uint8_t> BuildStopCommand();

private:
    static uint16_t ModbusCrc16(const uint8_t *data, std::size_t size);
    static int16_t ReadInt16Le(const uint8_t *data);
    static float ReadFloatBe(const uint8_t *data);
    static bool IsPlausibleSample(const FtSensorSample &sample);

    std::vector<uint8_t> buffer_;
    uint32_t frame_error_count_{0};
    int reporting_hz_{1000};
};

} // namespace smrcore::peripherals::protocols::ft_sensor::xjc
