/**
 * @file xjc_reader.cpp
 * @brief 鑫精诚力传感器串口读取实现
 */

#include "protocols/ft_sensor/xjc/xjc_reader.hpp"

#include <chrono>
#include <thread>

namespace smrcore::peripherals::protocols::ft_sensor::xjc
{

XjcReader::~XjcReader() { Close(); }

bool XjcReader::Open(const XjcReaderConfig &config)
{
    Close();
    active_reporting_hz_ = config.active_reporting_hz;
    if (!parser_.SetReportingHz(active_reporting_hz_))
    {
        return false;
    }
    return reader_.Open(config);
}

bool XjcReader::StartStreaming()
{
    if (!reader_.IsOpen())
    {
        return false;
    }

    const auto command = XjcParser::BuildStartCommand(active_reporting_hz_);
    if (command.empty() ||
        !reader_.Serial().WriteAll(command.data(), command.size()))
    {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    reader_.Serial().FlushInput();
    return true;
}

bool XjcReader::StopStreaming()
{
    if (!reader_.IsOpen())
    {
        return true;
    }

    const auto command = XjcParser::BuildStopCommand();
    return reader_.Serial().WriteAll(command.data(), command.size());
}

bool XjcReader::Close()
{
    bool ok = true;
    if (reader_.IsOpen())
    {
        ok = StopStreaming();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    reader_.Close();
    return ok;
}

bool XjcReader::IsOpen() const { return reader_.IsOpen(); }

std::vector<FtSensorSample> XjcReader::ReadAvailable(int timeout_ms,
                                                     std::size_t max_bytes,
                                                     std::size_t max_frames,
                                                     double timestamp_sec,
                                                     bool *disconnected)
{
    return reader_.ReadAvailable(parser_, timeout_ms, max_bytes, max_frames,
                                 timestamp_sec, disconnected);
}

} // namespace smrcore::peripherals::protocols::ft_sensor::xjc
