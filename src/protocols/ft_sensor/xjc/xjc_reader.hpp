/**
 * @file xjc_reader.hpp
 * @brief 鑫精诚力传感器串口读取（品牌协议层）
 *
 * 主动上报模式：发送 Modbus 启流命令后，传感器按频率持续推送
 * 16 字节 int16 帧或 28 字节 float 帧。
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include "protocols/ft_sensor/serial_reader.hpp"
#include "protocols/ft_sensor/xjc/xjc_parser.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace smrcore::peripherals::protocols::ft_sensor::xjc
{

/** @brief XJC 读取器配置 */
struct XjcReaderConfig : SerialReaderConfig
{
    /** 主动上报频率，支持 100 / 250 / 500 / 1000 Hz */
    int active_reporting_hz{1000};
};

/**
 * @class XjcReader
 * @brief 鑫精诚六维力传感器：开串口、启停主动上报、批量读帧
 */
class XjcReader
{
public:
    XjcReader() = default;
    ~XjcReader();

    XjcReader(const XjcReader &) = delete;
    XjcReader &operator=(const XjcReader &) = delete;

    bool Open(const XjcReaderConfig &config);
    bool StartStreaming();
    bool StopStreaming();
    bool Close();
    bool IsOpen() const;

    std::vector<FtSensorSample>
    ReadAvailable(int timeout_ms, std::size_t max_bytes, std::size_t max_frames,
                  double timestamp_sec, bool *disconnected = nullptr);

    uint32_t frame_error_count() const { return parser_.frame_error_count(); }
    void ResetParser() { parser_.Reset(); }

private:
    SerialReader reader_;
    XjcParser parser_;
    int active_reporting_hz_{1000};
};

} // namespace smrcore::peripherals::protocols::ft_sensor::xjc
