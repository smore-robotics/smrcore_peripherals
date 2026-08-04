/**
 * @file xjc_parser.cpp
 * @brief 鑫精诚力传感器串口帧解析实现
 */

#include "protocols/ft_sensor/xjc/xjc_parser.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace smrcore::peripherals::protocols::ft_sensor::xjc
{

namespace
{

constexpr uint8_t kHeader0 = 0x20;
constexpr uint8_t kHeader1 = 0x4e;
constexpr std::array<uint8_t, 2> kHeader = {kHeader0, kHeader1};

int ReportingHzToRegisterValue(int reporting_hz)
{
    switch (reporting_hz)
    {
    case 100:
        return 0x0000;
    case 250:
        return 0x0001;
    case 500:
        return 0x0002;
    case 1000:
        return 0x0003;
    default:
        return -1;
    }
}

} // namespace

bool XjcParser::SetReportingHz(int reporting_hz)
{
    if (ReportingHzToRegisterValue(reporting_hz) < 0)
    {
        return false;
    }
    reporting_hz_ = reporting_hz;
    Reset();
    return true;
}

std::vector<FtSensorSample>
XjcParser::Feed(const uint8_t *data, std::size_t size, double timestamp_sec)
{
    std::vector<FtSensorSample> samples;
    if (data == nullptr || size == 0)
    {
        return samples;
    }

    buffer_.insert(buffer_.end(), data, data + size);
    const std::size_t frame_size =
        reporting_hz_ == 1000 ? kFloatFrameSize : kIntegerFrameSize;
    while (true)
    {
        const auto header_it = std::search(buffer_.begin(), buffer_.end(),
                                           kHeader.begin(), kHeader.end());
        if (header_it == buffer_.end())
        {
            if (buffer_.size() > 1)
            {
                ++frame_error_count_;
                const uint8_t last = buffer_.back();
                buffer_.clear();
                if (last == kHeader0)
                {
                    buffer_.push_back(last);
                }
            }
            return samples;
        }

        const auto header_offset =
            static_cast<std::size_t>(header_it - buffer_.begin());
        if (header_offset > 0)
        {
            ++frame_error_count_;
            buffer_.erase(buffer_.begin(), buffer_.begin() + header_offset);
        }
        if (buffer_.size() < frame_size)
        {
            return samples;
        }

        const uint16_t expected_crc =
            ModbusCrc16(buffer_.data(), frame_size - 2);
        const uint16_t received_crc =
            static_cast<uint16_t>(buffer_[frame_size - 2]) |
            (static_cast<uint16_t>(buffer_[frame_size - 1]) << 8);
        if (expected_crc != received_crc)
        {
            ++frame_error_count_;
            buffer_.erase(buffer_.begin());
            continue;
        }

        FtSensorSample sample;
        if (reporting_hz_ == 1000)
        {
            sample.fx = static_cast<double>(
                ReadFloatBe(buffer_.data() + kPayloadOffset));
            sample.fy = static_cast<double>(
                ReadFloatBe(buffer_.data() + kPayloadOffset + 4));
            sample.fz = static_cast<double>(
                ReadFloatBe(buffer_.data() + kPayloadOffset + 8));
            sample.tx = static_cast<double>(
                ReadFloatBe(buffer_.data() + kPayloadOffset + 12));
            sample.ty = static_cast<double>(
                ReadFloatBe(buffer_.data() + kPayloadOffset + 16));
            sample.tz = static_cast<double>(
                ReadFloatBe(buffer_.data() + kPayloadOffset + 20));
        }
        else
        {
            sample.fx = static_cast<double>(
                            ReadInt16Le(buffer_.data() + kPayloadOffset)) /
                        100.0;
            sample.fy = static_cast<double>(
                            ReadInt16Le(buffer_.data() + kPayloadOffset + 2)) /
                        100.0;
            sample.fz = static_cast<double>(
                            ReadInt16Le(buffer_.data() + kPayloadOffset + 4)) /
                        100.0;
            sample.tx = static_cast<double>(
                            ReadInt16Le(buffer_.data() + kPayloadOffset + 6)) /
                        1000.0;
            sample.ty = static_cast<double>(
                            ReadInt16Le(buffer_.data() + kPayloadOffset + 8)) /
                        1000.0;
            sample.tz = static_cast<double>(
                            ReadInt16Le(buffer_.data() + kPayloadOffset + 10)) /
                        1000.0;
        }
        sample.timestamp_sec = timestamp_sec;

        if (!IsPlausibleSample(sample))
        {
            ++frame_error_count_;
            buffer_.erase(buffer_.begin());
            continue;
        }

        samples.push_back(sample);
        buffer_.erase(buffer_.begin(), buffer_.begin() + frame_size);
    }
}

std::optional<FtSensorSample> XjcParser::DrainLatest(const uint8_t *data,
                                                     std::size_t size,
                                                     double timestamp_sec)
{
    auto samples = Feed(data, size, timestamp_sec);
    if (samples.empty())
    {
        return std::nullopt;
    }
    return samples.back();
}

void XjcParser::Reset()
{
    buffer_.clear();
    frame_error_count_ = 0;
}

std::vector<uint8_t> XjcParser::BuildStartCommand(int reporting_hz)
{
    const int value = ReportingHzToRegisterValue(reporting_hz);
    if (value < 0)
    {
        return {};
    }

    const std::array<uint8_t, 9> frame_wo_crc = {
        0x01,
        0x10,
        0x01,
        0x9a,
        0x00,
        0x01,
        0x02,
        static_cast<uint8_t>((value >> 8) & 0xff),
        static_cast<uint8_t>(value & 0xff)};
    const uint16_t crc = ModbusCrc16(frame_wo_crc.data(), frame_wo_crc.size());
    std::vector<uint8_t> command(frame_wo_crc.begin(), frame_wo_crc.end());
    command.push_back(static_cast<uint8_t>(crc & 0xff));
    command.push_back(static_cast<uint8_t>((crc >> 8) & 0xff));
    return command;
}

std::vector<uint8_t> XjcParser::BuildStopCommand()
{
    return std::vector<uint8_t>(12, 0xff);
}

uint16_t XjcParser::ModbusCrc16(const uint8_t *data, std::size_t size)
{
    uint16_t crc = 0xffff;
    for (std::size_t i = 0; i < size; ++i)
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
        {
            if ((crc & 0x0001) != 0)
            {
                crc = static_cast<uint16_t>((crc >> 1) ^ 0xa001);
            }
            else
            {
                crc = static_cast<uint16_t>(crc >> 1);
            }
        }
    }
    return crc;
}

int16_t XjcParser::ReadInt16Le(const uint8_t *data)
{
    const uint16_t word =
        static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
    int16_t value = 0;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}

float XjcParser::ReadFloatBe(const uint8_t *data)
{
    const uint32_t word = (static_cast<uint32_t>(data[0]) << 24) |
                          (static_cast<uint32_t>(data[1]) << 16) |
                          (static_cast<uint32_t>(data[2]) << 8) |
                          static_cast<uint32_t>(data[3]);
    float value = 0.0F;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}

bool XjcParser::IsPlausibleSample(const FtSensorSample &sample)
{
    const auto finite = [](double value)
    { return std::isfinite(value) && !std::isnan(value); };
    if (!finite(sample.fx) || !finite(sample.fy) || !finite(sample.fz) ||
        !finite(sample.tx) || !finite(sample.ty) || !finite(sample.tz))
    {
        return false;
    }

    constexpr double kMaxForce = 10000.0;
    constexpr double kMaxTorque = 1000.0;
    return std::abs(sample.fx) <= kMaxForce &&
           std::abs(sample.fy) <= kMaxForce &&
           std::abs(sample.fz) <= kMaxForce &&
           std::abs(sample.tx) <= kMaxTorque &&
           std::abs(sample.ty) <= kMaxTorque &&
           std::abs(sample.tz) <= kMaxTorque;
}

} // namespace smrcore::peripherals::protocols::ft_sensor::xjc
