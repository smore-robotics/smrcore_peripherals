/**
 * @file serial_reader.hpp
 * @brief 力传感器通用串口批量读取（协议层）
 *
 * 负责串口打开/关闭与「读字节 → 喂给 Parser → 收集样本」循环，
 * 与具体品牌帧格式解耦。品牌差异由 `Parser::Feed()` 实现。
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include "peripherals/types.hpp"
#include "protocols/ft_sensor/port/posix_serial_port.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace smrcore::peripherals::protocols::ft_sensor
{

/**
 * @struct SerialReaderConfig
 * @brief 串口读取器配置
 */
struct SerialReaderConfig
{
    std::string serial_port;           ///< 设备路径
    int baud_rate{460800};             ///< 波特率
    std::size_t read_buffer_size{512}; ///< 单次 read 缓冲区大小
};

/**
 * @class SerialReader
 * @brief 品牌无关的串口采样读取器
 *
 * 典型用法：由品牌 Reader（如 `kunwei::KunweiReader`）持有 `SerialReader`
 * 与对应 `Parser`，在 `ReadAvailable()` 中传入 parser 引用。
 *
 * Parser 需满足：
 * - `std::vector<FtSensorSample> Feed(const uint8_t*, std::size_t, double)`
 */
class SerialReader
{
public:
    SerialReader() = default;
    ~SerialReader();

    SerialReader(const SerialReader &) = delete;
    SerialReader &operator=(const SerialReader &) = delete;

    /**
     * @brief 打开串口并重置内部状态
     * @param config 串口参数；`read_buffer_size==0` 时回退为 512
     */
    bool Open(const SerialReaderConfig &config);

    /** @brief 关闭串口（可重复调用） */
    void Close();

    /** @return 底层串口是否已打开 */
    bool IsOpen() const;

    /** @brief 访问底层串口，供品牌层发送启停流等控制命令 */
    port::PosixSerialPort &Serial() { return serial_; }
    const port::PosixSerialPort &Serial() const { return serial_; }

    /**
     * @brief 在限时/限字节/限帧约束下批量读取并解析
     *
     * 首次 `ReadSome` 使用 `timeout_ms`；同一次调用内后续读使用 0 超时以 drain。
     *
     * @tparam Parser 品牌帧解析器，需提供 `Feed(data, size, timestamp_sec)`
     * @param parser 解析器实例（由调用方持有，可跨调用复用）
     * @param timeout_ms 首次等待毫秒数
     * @param max_bytes 本次最多读取的原始字节数
     * @param max_frames 本次最多返回的样本帧数
     * @param timestamp_sec 写入每个 `FtSensorSample::timestamp_sec`
     * @param disconnected 可选；串口断开时置 true
     * @return 按解析顺序排列的样本；可能为空
     */
    template <typename Parser>
    std::vector<FtSensorSample> ReadAvailable(Parser &parser, int timeout_ms,
                                              std::size_t max_bytes,
                                              std::size_t max_frames,
                                              double timestamp_sec,
                                              bool *disconnected = nullptr);

private:
    SerialReaderConfig config_;
    port::PosixSerialPort serial_;
};

template <typename Parser>
std::vector<FtSensorSample> SerialReader::ReadAvailable(
    Parser &parser, int timeout_ms, std::size_t max_bytes,
    std::size_t max_frames, double timestamp_sec, bool *disconnected)
{
    if (disconnected)
    {
        *disconnected = false;
    }

    std::vector<FtSensorSample> out;
    if (!serial_.IsOpen() || max_bytes == 0 || max_frames == 0)
    {
        if (disconnected && !serial_.IsOpen())
        {
            *disconnected = true;
        }
        return out;
    }

    std::vector<uint8_t> buffer(config_.read_buffer_size, 0);
    std::size_t total_read = 0;
    while (total_read < max_bytes && out.size() < max_frames)
    {
        const std::size_t to_read =
            std::min(buffer.size(), max_bytes - total_read);
        bool read_disconnected = false;
        const int n = serial_.ReadSome(buffer.data(), to_read,
                                       total_read == 0 ? timeout_ms : 0,
                                       &read_disconnected);
        if (read_disconnected)
        {
            if (disconnected)
            {
                *disconnected = true;
            }
            break;
        }
        if (n <= 0)
        {
            break;
        }

        total_read += static_cast<std::size_t>(n);
        auto parsed = parser.Feed(buffer.data(), static_cast<std::size_t>(n),
                                  timestamp_sec);
        for (auto &sample : parsed)
        {
            if (out.size() >= max_frames)
            {
                break;
            }
            out.push_back(sample);
        }
    }
    return out;
}

} // namespace smrcore::peripherals::protocols::ft_sensor
