/**
 * @file kunwei_parser.cpp
 * @brief 坤维力传感器串口帧解析实现
 */

#include "protocols/ft_sensor/kunwei/kunwei_parser.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace smrcore::peripherals::protocols::ft_sensor::kunwei
{

namespace
{

constexpr uint8_t kHeader0 = 0x48;
constexpr uint8_t kHeader1 = 0xaa;
constexpr uint8_t kFooter0 = 0x0d;
constexpr uint8_t kFooter1 = 0x0a;
constexpr double kGravity = 9.81;
constexpr std::array<uint8_t, 2> kHeader = {kHeader0, kHeader1};

} // namespace

std::vector<FtSensorSample> KunweiParser::Feed(const uint8_t *data,
                                               std::size_t size,
                                               double timestamp_sec)
{
    std::vector<FtSensorSample> samples;
    if (data == nullptr || size == 0)
    {
        return samples;
    }

    buffer_.insert(buffer_.end(), data, data + size);
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

        if (buffer_.size() < kFrameSize)
        {
            return samples;
        }

        if (!IsFooter(buffer_, kFrameSize - 2))
        {
            ++frame_error_count_;
            buffer_.erase(buffer_.begin());
            continue;
        }

        const double fx = static_cast<double>(ReadFloatLe(buffer_.data() + 2)) *
                          kGravity;
        const double fy = static_cast<double>(ReadFloatLe(buffer_.data() + 6)) *
                          kGravity;
        const double fz = static_cast<double>(ReadFloatLe(buffer_.data() + 10)) *
                          kGravity;
        const double tx = static_cast<double>(ReadFloatLe(buffer_.data() + 14)) *
                          kGravity;
        const double ty = static_cast<double>(ReadFloatLe(buffer_.data() + 18)) *
                          kGravity;
        const double tz = static_cast<double>(ReadFloatLe(buffer_.data() + 22)) *
                          kGravity;

        FtSensorSample sample;
        sample.fx = fx;
        sample.fy = fy;
        sample.fz = fz;
        sample.tx = tx;
        sample.ty = ty;
        sample.tz = tz;
        sample.timestamp_sec = timestamp_sec;
        samples.push_back(sample);
        buffer_.erase(buffer_.begin(), buffer_.begin() + kFrameSize);
    }
}

std::optional<FtSensorSample>
KunweiParser::DrainLatest(const uint8_t *data, std::size_t size,
                          double timestamp_sec)
{
    auto samples = Feed(data, size, timestamp_sec);
    if (samples.empty())
    {
        return std::nullopt;
    }
    return samples.back();
}

void KunweiParser::Reset()
{
    buffer_.clear();
    frame_error_count_ = 0;
}

bool KunweiParser::IsFooter(const std::vector<uint8_t> &buffer,
                            std::size_t offset)
{
    return offset + 1 < buffer.size() && buffer[offset] == kFooter0 &&
           buffer[offset + 1] == kFooter1;
}

float KunweiParser::ReadFloatLe(const uint8_t *data)
{
    uint32_t word = static_cast<uint32_t>(data[0]) |
                    (static_cast<uint32_t>(data[1]) << 8) |
                    (static_cast<uint32_t>(data[2]) << 16) |
                    (static_cast<uint32_t>(data[3]) << 24);
    float value = 0.0F;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}

} // namespace smrcore::peripherals::protocols::ft_sensor::kunwei
