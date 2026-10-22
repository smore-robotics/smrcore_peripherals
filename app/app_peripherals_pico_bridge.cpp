#include "peripherals/pico/teleop.hpp"
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
namespace
{
using Options = smrcore::peripherals::PicoTeleopOptions;
constexpr double kMinimumMappingScale = 0.1;
constexpr double kMaximumMappingScale = 3.0;
smrcore::peripherals::PicoTeleop session;
void SignalHandler(int) { session.RequestStop(); }
void PrintUsage(const char *program)
{
    std::cout << "用法: " << program
              << " [--enable-motion] [--translation-only] [--room-fixed]"
                 " [--operator-frame same-side|face-to-face] [--port 8765]"
                 " [--motion-scale 1.0] [--translation-scale 0.8]"
                 " [--rotation-scale 1.0] [--j6-speed 0.8]"
                 " [--gripper-force 80] [--gripper-speed 100]"
                 " [--robot-ip ADDRESS]\n"
              << "--enable-motion 是必需的实机安全确认；控制链仅为 SDK "
                 "ServoP；按住 Grip 时手柄摇杆 Y 轴控制对应机械臂 J6。\n";
}

bool ParseOperatorFrame(const std::string &text, bool &operator_facing_robot)
{
    if (text == "same-side")
    {
        operator_facing_robot = false;
        return true;
    }
    if (text == "face-to-face")
    {
        operator_facing_robot = true;
        return true;
    }
    return false;
}

bool ParsePort(const std::string &text, uint16_t &port)
{
    try
    {
        const unsigned long value = std::stoul(text);
        if (value == 0 || value > 65535)
        {
            return false;
        }
        port = static_cast<uint16_t>(value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ParseMappingScale(const std::string &text, double &scale)
{
    try
    {
        size_t parsed = 0;
        const double value = std::stod(text, &parsed);
        if (parsed != text.size() || !std::isfinite(value) ||
            value < kMinimumMappingScale || value > kMaximumMappingScale)
        {
            return false;
        }
        scale = value;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ParsePercent(const std::string &text, uint16_t minimum, uint16_t &value)
{
    try
    {
        size_t parsed = 0;
        const unsigned long parsed_value = std::stoul(text, &parsed);
        if (parsed != text.size() || parsed_value < minimum ||
            parsed_value > 100)
        {
            return false;
        }
        value = static_cast<uint16_t>(parsed_value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ParseOptions(int argc, char **argv, Options &options)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--enable-motion")
        {
            options.enable_motion = true;
        }
        else if (argument == "--translation-only")
        {
            options.enable_rotation = false;
        }
        else if (argument == "--room-fixed")
        {
            options.head_yaw_clutched = false;
        }
        else if (argument == "--operator-frame" && i + 1 < argc)
        {
            if (!ParseOperatorFrame(argv[++i], options.operator_facing_robot))
            {
                return false;
            }
        }
        else if (argument == "--port" && i + 1 < argc)
        {
            if (!ParsePort(argv[++i], options.port))
            {
                return false;
            }
        }
        else if (argument == "--motion-scale" && i + 1 < argc)
        {
            double scale = 0.0;
            if (!ParseMappingScale(argv[++i], scale))
            {
                return false;
            }
            options.translation_scale = scale;
            options.rotation_scale = scale;
        }
        else if (argument == "--translation-scale" && i + 1 < argc)
        {
            if (!ParseMappingScale(argv[++i], options.translation_scale))
            {
                return false;
            }
        }
        else if (argument == "--rotation-scale" && i + 1 < argc)
        {
            if (!ParseMappingScale(argv[++i], options.rotation_scale))
            {
                return false;
            }
        }
        else if (argument == "--j6-speed" && i + 1 < argc)
        {
            if (!ParseMappingScale(argv[++i], options.j6_thumbstick_speed))
            {
                return false;
            }
        }
        else if (argument == "--gripper-force" && i + 1 < argc)
        {
            if (!ParsePercent(argv[++i], 20, options.gripper_force))
            {
                return false;
            }
        }
        else if (argument == "--gripper-speed" && i + 1 < argc)
        {
            if (!ParsePercent(argv[++i], 1, options.gripper_speed))
            {
                return false;
            }
        }
        else if (argument == "--robot-ip" && i + 1 < argc)
        {
            options.robot_ip = argv[++i];
        }
        else if (argument == "--help" || argument == "-h")
        {
            PrintUsage(argv[0]);
            std::exit(0);
        }
        else
        {
            return false;
        }
    }
    return true;
}

} // namespace
int main(int argc, char **argv)
{
    Options options;
    options.translation_scale = .8;
    options.rotation_scale = 1.;
    options.j6_thumbstick_speed = .8;
    if (!ParseOptions(argc, argv, options))
    {
        PrintUsage(argv[0]);
        return 2;
    }
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    return session.Run(options);
}
