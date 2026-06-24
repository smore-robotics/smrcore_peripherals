#include "protocols/ft_sensor/xjc/xjc_parser.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

using smrcore::peripherals::protocols::ft_sensor::xjc::XjcParser;

namespace
{

std::vector<uint8_t> CapturedFrame()
{
    return {
        0x76, 0xcf, 0xe5, 0xa5, 0x66, 0x20, 0x4e, 0x3e, 0xc4, 0xc2, 0x8a, 0xbf,
        0xe8, 0x8c, 0x6d, 0xbf, 0x9a, 0x96, 0x11, 0xbb, 0x25, 0xca, 0xdb, 0x3b,
        0x0b, 0x5e, 0x57, 0x3c,
    };
}

} // namespace

int main()
{
    XjcParser parser;
    const auto frame = CapturedFrame();
    assert(frame.size() == XjcParser::kFrameSize);
    assert(frame.back() == XjcParser::kFooterByte);

    auto samples = parser.Feed(frame.data(), frame.size(), 1.0);
    assert(samples.size() == 1);
    assert(std::abs(samples[0].fx - 0.2013) < 1e-3);
    assert(std::abs(samples[0].fy + 1.0841) < 1e-3);
    assert(std::abs(samples[0].fz + 0.9279) < 1e-3);

    const auto start_1000 = XjcParser::BuildStartCommand(1000);
    assert(start_1000.size() == 11);
    assert(start_1000[7] == 0x00);
    assert(start_1000[8] == 0x03);
    assert(start_1000[9] == 0xeb);
    assert(start_1000[10] == 0x6b);

    assert(parser.DrainLatest(frame.data(), frame.size(), 2.0).has_value());
    return 0;
}
