#include "peripherals/peripherals.hpp"
#include "sdk/robot.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace
{

rcore::GripperCommand ToSdk(smrcore::peripherals::GripperCommand command)
{
    return command == smrcore::peripherals::GripperCommand::Close
               ? rcore::GripperCommand::Close
               : rcore::GripperCommand::Open;
}

} // namespace

int main(int argc, char **argv)
{
    std::string robot_ip;
    smrcore::peripherals::SpaceMouseOptions options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--robot" && i + 1 < argc)
        {
            robot_ip = argv[++i];
        }
        else if (arg == "--device" && i + 1 < argc)
        {
            options.device_path = argv[++i];
        }
        else if (arg == "--sample-rate" && i + 1 < argc)
        {
            options.sample_rate_hz = std::atof(argv[++i]);
        }
    }

    rcore::sdk::Robot robot;
    if (!robot.Initialize(robot_ip))
    {
        std::cerr << "failed to initialize robot SDK" << std::endl;
        return 1;
    }

    smrcore::peripherals::SpaceMouse spacemouse;
    if (!spacemouse.Initialize(options) || !spacemouse.Start())
    {
        std::cerr << "failed to start SpaceMouse reader" << std::endl;
        return 1;
    }

    auto peripheral = robot.Peripheral();
    while (true)
    {
        if (auto sample = spacemouse.GetSample())
        {
            rcore::SpaceMouseSample out;
            out.x = sample->x;
            out.y = sample->y;
            out.z = sample->z;
            out.roll = sample->roll;
            out.pitch = sample->pitch;
            out.yaw = sample->yaw;
            out.gripper_command = ToSdk(sample->gripper_command);
            out.timestamp_sec = sample->timestamp_sec;
            auto result = peripheral.UpdateSpaceMouseSample(out);
            if (!result.IsSuccess())
            {
                std::cerr << "UpdateSpaceMouseSample failed: "
                          << result.GetErrorCode() << " "
                          << result.GetErrorMsg() << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
