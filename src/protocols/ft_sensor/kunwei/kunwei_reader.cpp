/**
 * @file kunwei_reader.cpp
 * @brief 坤维力传感器串口读取实现
 */

#include "protocols/ft_sensor/kunwei/kunwei_reader.hpp"

#include <array>

namespace smrcore::peripherals::protocols::ft_sensor::kunwei
{

namespace
{

constexpr std::array<uint8_t, 4> kStartCommand = {0x48, 0xaa, 0x0d, 0x0a};
constexpr std::array<uint8_t, 4> kStopCommand = {0x43, 0xaa, 0x0d, 0x0a};

} // namespace

KunweiReader::~KunweiReader() { Close(); }

bool KunweiReader::Open(const KunweiReaderConfig &config)
{
    Close();
    parser_.Reset();
    return reader_.Open(config);
}

bool KunweiReader::StartStreaming()
{
    return reader_.IsOpen() &&
           reader_.Serial().WriteAll(kStartCommand.data(), kStartCommand.size());
}

bool KunweiReader::StopStreaming()
{
    return !reader_.IsOpen() ||
           reader_.Serial().WriteAll(kStopCommand.data(), kStopCommand.size());
}

bool KunweiReader::Close()
{
    bool ok = true;
    if (reader_.IsOpen())
    {
        ok = StopStreaming();
    }
    reader_.Close();
    return ok;
}

bool KunweiReader::IsOpen() const { return reader_.IsOpen(); }

std::vector<FtSensorSample>
KunweiReader::ReadAvailable(int timeout_ms, std::size_t max_bytes,
                            std::size_t max_frames, double timestamp_sec,
                            bool *disconnected)
{
    return reader_.ReadAvailable(parser_, timeout_ms, max_bytes, max_frames,
                                 timestamp_sec, disconnected);
}

} // namespace smrcore::peripherals::protocols::ft_sensor::kunwei
