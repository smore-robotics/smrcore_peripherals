#include "protocols/ft_sensor/xjc/xjc_parser.hpp"

#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <vector>

using smrcore::peripherals::protocols::ft_sensor::xjc::XjcParser;

namespace
{

std::vector<uint8_t> CapturedIntegerFrame()
{
    return {0x20, 0x4e, 0xaf, 0xff, 0x4c, 0x00, 0x9e, 0xff,
            0x19, 0x00, 0x05, 0x00, 0x5f, 0x00, 0xb8, 0x5c};
}

std::vector<uint8_t> CapturedFloatFrame()
{
    return {0x20, 0x4e, 0xbf, 0x4d, 0x64, 0x5f, 0x3f, 0x3f, 0xcc, 0x25,
            0xbf, 0x7a, 0x3b, 0x2f, 0x3c, 0xcb, 0xf0, 0xe1, 0x3b, 0xd5,
            0x12, 0xae, 0x3d, 0xc4, 0xed, 0x2d, 0xe7, 0x28};
}

void Check(bool condition)
{
    if (!condition)
    {
        std::abort();
    }
}

void ExpectNear(double actual, double expected, double tolerance = 1e-6)
{
    Check(std::abs(actual - expected) < tolerance);
}

} // namespace

int main()
{
    XjcParser parser;

    Check(parser.SetReportingHz(100));
    const auto integer_frame = CapturedIntegerFrame();
    auto integer_samples = parser.Feed(integer_frame.data(), 7, 1.0);
    Check(integer_samples.empty());
    integer_samples =
        parser.Feed(integer_frame.data() + 7, integer_frame.size() - 7, 1.0);
    Check(integer_samples.size() == 1);
    ExpectNear(integer_samples[0].fx, -0.81);
    ExpectNear(integer_samples[0].fy, 0.76);
    ExpectNear(integer_samples[0].fz, -0.98);
    ExpectNear(integer_samples[0].tx, 0.025);
    ExpectNear(integer_samples[0].ty, 0.005);
    ExpectNear(integer_samples[0].tz, 0.095);

    Check(parser.SetReportingHz(1000));
    const auto float_frame = CapturedFloatFrame();
    const std::vector<uint8_t> prefixed = {0x99, 0x20};
    auto float_samples = parser.Feed(prefixed.data(), prefixed.size(), 2.0);
    Check(float_samples.empty());
    float_samples =
        parser.Feed(float_frame.data() + 1, float_frame.size() - 1, 2.0);
    Check(float_samples.size() == 1);
    ExpectNear(float_samples[0].fx, -0.8023127913);
    ExpectNear(float_samples[0].fy, 0.7492087483);
    ExpectNear(float_samples[0].fz, -0.9774655700);
    ExpectNear(float_samples[0].tx, 0.0248951335);
    ExpectNear(float_samples[0].ty, 0.0065024709);
    ExpectNear(float_samples[0].tz, 0.0961555019);

    auto corrupted = float_frame;
    corrupted[10] ^= 0x01;
    Check(parser.Feed(corrupted.data(), corrupted.size(), 3.0).empty());
    Check(parser.frame_error_count() > 0);

    const auto start_1000 = XjcParser::BuildStartCommand(1000);
    Check(start_1000.size() == 11);
    Check(start_1000[7] == 0x00);
    Check(start_1000[8] == 0x03);
    Check(start_1000[9] == 0xeb);
    Check(start_1000[10] == 0x6b);
    Check(!parser.SetReportingHz(200));

    Check(XjcParser::BuildStopCommand() == std::vector<uint8_t>(12, 0xff));
    return 0;
}
