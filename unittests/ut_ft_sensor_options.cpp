#include "peripherals/ft_sensor/ft_sensor.hpp"

#include <cassert>

int main()
{
    smrcore::peripherals::FtSensorOptions options;
    options.serial_port = "/dev/ttyUSB1";
    options.sensor_type = "xjc_serial";
    options.baud_rate = 115200;
    options.read_buffer_size = 1024;
    options.sample_rate_hz = 500.0;
    options.stale_timeout_ms = 30;

    smrcore::peripherals::FtSensor sensor;
    assert(sensor.Initialize(options));

    const auto info = sensor.GetInfo();
    assert(info.path == "/dev/ttyUSB1");
    assert(info.model == "XJC F/T");
    assert(info.sample_rate_hz == 500.0);
    assert(!info.connected);

    return 0;
}
