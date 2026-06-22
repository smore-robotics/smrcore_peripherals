#include "peripherals/peripherals.hpp"

#include <iostream>
#include <string>

namespace
{

bool ParseOptions(int argc, char **argv, std::string &serial_port)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--serial-port" && i + 1 < argc)
        {
            serial_port = argv[++i];
        }
        else
        {
            std::cerr << "unknown argument: " << arg << '\n';
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    std::string serial_port = "/dev/ttyUSB0";
    if (!ParseOptions(argc, argv, serial_port))
    {
        return 1;
    }

    smrcore::peripherals::SpaceMouse spacemouse;
    smrcore::peripherals::SpaceMouseOptions sm_options;
    (void)spacemouse.Initialize(sm_options);
    auto sm_info = spacemouse.GetInfo();
    std::cout << "spacemouse model=" << sm_info.model
              << " rate_hz=" << sm_info.sample_rate_hz
              << " connected=" << sm_info.connected << '\n';

    smrcore::peripherals::FtSensor ft_sensor;
    smrcore::peripherals::FtSensorOptions ft_options;
    ft_options.serial_port = serial_port;
    (void)ft_sensor.Initialize(ft_options);
    auto ft_info = ft_sensor.GetInfo();
    std::cout << "ft_sensor model=" << ft_info.model
              << " rate_hz=" << ft_info.sample_rate_hz
              << " connected=" << ft_info.connected << '\n';
    return 0;
}
