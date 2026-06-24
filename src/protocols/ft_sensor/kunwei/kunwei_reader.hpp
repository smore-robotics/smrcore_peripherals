/**
 * @file kunwei_reader.hpp
 * @brief 坤维力传感器串口读取（品牌协议层）
 *
 * 组合通用 `SerialReader` 与 `KunweiParser`，并发送坤维专有启停流命令。
 * 新增其他品牌时，在 `protocols/ft_sensor/<brand>/` 下实现对应 Parser 与 Reader。
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include "protocols/ft_sensor/kunwei/kunwei_parser.hpp"
#include "protocols/ft_sensor/serial_reader.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace smrcore::peripherals::protocols::ft_sensor::kunwei
{

/** @brief 坤维读取器配置（与 `SerialReaderConfig` 字段一致） */
using KunweiReaderConfig = SerialReaderConfig;

/**
 * @class KunweiReader
 * @brief 坤维六维力传感器：开串口、启停流、批量读帧
 *
 * 启流：`0x48 0xaa 0x0d 0x0a`；停流：`0x43 0xaa 0x0d 0x0a`。
 * `ReadAvailable()` 委托给 `SerialReader` + `KunweiParser`。
 */
class KunweiReader
{
public:
    KunweiReader() = default;
    ~KunweiReader();

    KunweiReader(const KunweiReader &) = delete;
    KunweiReader &operator=(const KunweiReader &) = delete;

    /** @brief 打开串口并重置解析器状态 */
    bool Open(const KunweiReaderConfig &config);

    /** @brief 发送坤维启流命令 */
    bool StartStreaming();

    /** @brief 发送坤维停流命令 */
    bool StopStreaming();

    /** @brief 停流后关闭串口 */
    bool Close();

    bool IsOpen() const;

    /**
     * @brief 批量读取可用样本
     * @see SerialReader::ReadAvailable
     */
    std::vector<FtSensorSample> ReadAvailable(int timeout_ms,
                                              std::size_t max_bytes,
                                              std::size_t max_frames,
                                              double timestamp_sec,
                                              bool *disconnected = nullptr);

    uint32_t frame_error_count() const { return parser_.frame_error_count(); }
    void ResetParser() { parser_.Reset(); }

private:
    SerialReader reader_;
    KunweiParser parser_;
};

} // namespace smrcore::peripherals::protocols::ft_sensor::kunwei
