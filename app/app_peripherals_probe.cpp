#include "peripherals/peripherals.hpp"

#include <iostream>

int main()
{
    smrcore::peripherals::SpaceMouse spacemouse;
    smrcore::peripherals::SpaceMouseOptions sm_options;
    (void)spacemouse.Initialize(sm_options);
    auto sm_info = spacemouse.GetInfo();
    std::cout << "spacemouse model=" << sm_info.model
              << " rate_hz=" << sm_info.sample_rate_hz
              << " connected=" << sm_info.connected << '\n';

    smrcore::peripherals::FtSensor ft_sensor;
    smrcore::peripherals::FtSensorOptions ft_options;
    ft_options.serial_port = "/dev/ttyUSB0";
    (void)ft_sensor.Initialize(ft_options);
    auto ft_info = ft_sensor.GetInfo();
    std::cout << "ft_sensor model=" << ft_info.model
              << " rate_hz=" << ft_info.sample_rate_hz
              << " connected=" << ft_info.connected << '\n';
    return 0;
}
