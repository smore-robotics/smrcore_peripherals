#include "protocols/ft_sensor/kunwei/kunwei_parser.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

using smrcore::peripherals::protocols::ft_sensor::kunwei::KunweiParser;

namespace
{

void AppendFloat(std::vector<uint8_t> &frame, float value)
{
    uint32_t word = 0;
    std::memcpy(&word, &value, sizeof(value));
    frame.push_back(static_cast<uint8_t>(word & 0xffu));
    frame.push_back(static_cast<uint8_t>((word >> 8) & 0xffu));
    frame.push_back(static_cast<uint8_t>((word >> 16) & 0xffu));
    frame.push_back(static_cast<uint8_t>((word >> 24) & 0xffu));
}

std::vector<uint8_t> MakeFrame(float base)
{
    std::vector<uint8_t> frame;
    frame.push_back(0x48);
    frame.push_back(0xaa);
    for (int i = 0; i < 6; ++i)
    {
        AppendFloat(frame, base + static_cast<float>(i));
    }
    frame.push_back(0x0d);
    frame.push_back(0x0a);
    return frame;
}

} // namespace

int main()
{
    KunweiParser parser;
    const auto frame = MakeFrame(1.0F);
    auto samples = parser.Feed(frame.data(), frame.size(), 1.0);
    assert(samples.size() == 1);
    assert(samples[0].fx > 0.0);

    assert(parser.DrainLatest(frame.data(), frame.size(), 2.0).has_value());
    return 0;
}
