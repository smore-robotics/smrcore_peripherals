/**
 * @file xjc_parser.hpp
 * @brief 鑫精诚（XJC）六维力传感器主动上报模式帧解析
 *
 * 实测 XJC 主动上报流为 28 字节帧（与启流 Modbus 命令配套）：
 * - 4 字节帧头（可变）
 * - 6× float32 LE（Fx,Fy,Fz,Mx,My,Mz），单位 N / N·m
 * - 末字节固定为 0x3C，用作帧同步
 *
 * 注：部分文档/示例中的 16 字节 int16 + Modbus CRC 格式与本机型固件不兼容。
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
 * @brief XJC 主动上报流式解析器（28 字节 float 帧）
 */
class XjcParser
{
public:
    static constexpr std::size_t kFrameSize = 28;
    static constexpr std::size_t kPayloadOffset = 4;
    static constexpr uint8_t kFooterByte = 0x3c;

    std::vector<FtSensorSample> Feed(const uint8_t *data, std::size_t size,
                                     double timestamp_sec);

    std::optional<FtSensorSample> DrainLatest(const uint8_t *data,
                                              std::size_t size,
                                              double timestamp_sec);

    uint32_t frame_error_count() const { return frame_error_count_; }

    void Reset();

    /** @brief 构建主动上报启流 Modbus 命令（写保持寄存器 0x019A） */
    static std::vector<uint8_t> BuildStartCommand(int reporting_hz);

    /** @brief 构建停流命令（12 字节 0xFF） */
    static std::vector<uint8_t> BuildStopCommand();

private:
    static uint16_t ModbusCrc16(const uint8_t *data, std::size_t size);
    static float ReadFloatLe(const uint8_t *data);
    static bool IsPlausibleSample(const FtSensorSample &sample);

    std::vector<uint8_t> buffer_;
    uint32_t frame_error_count_{0};
};

} // namespace smrcore::peripherals::protocols::ft_sensor::xjc
