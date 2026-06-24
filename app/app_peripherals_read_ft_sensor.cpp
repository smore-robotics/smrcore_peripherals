#include "peripherals/peripherals.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace
{

bool ParseOptions(int argc, char **argv, smrcore::peripherals::FtSensorOptions &options)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--serial-port" && i + 1 < argc)
        {
            options.serial_port = argv[++i];
        }
        else if (arg == "--sensor-type" && i + 1 < argc)
        {
            options.sensor_type = argv[++i];
        }
        else if (arg == "--baud-rate" && i + 1 < argc)
        {
            options.baud_rate = std::atoi(argv[++i]);
        }
        else if (arg == "--sample-rate" && i + 1 < argc)
        {
            options.sample_rate_hz = std::atof(argv[++i]);
        }
        else if (arg == "--stale-timeout-ms" && i + 1 < argc)
        {
            options.stale_timeout_ms = std::atoi(argv[++i]);
        }
        else if (arg == "--read-buffer-size" && i + 1 < argc)
        {
            options.read_buffer_size =
                static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10));
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
    smrcore::peripherals::FtSensorOptions options;
    options.serial_port = "/dev/ttyUSB0";
    options.sample_rate_hz = 1000.0;
    if (!ParseOptions(argc, argv, options))
    {
        return 1;
    }

    smrcore::peripherals::FtSensor peripheral;
    if (!peripheral.Initialize(options))
    {
        std::cerr << "failed to initialize F/T peripheral\n";
        return 1;
    }

    std::cout << "serial_port=" << options.serial_port
              << " baud_rate=" << options.baud_rate << " type=" << options.sensor_type
              << '\n';

    if (!peripheral.Start())
    {
        std::cerr << "failed to start F/T peripheral\n";
        return 1;
    }

    if (!peripheral.WaitForFirstSample())
    {
        const auto info = peripheral.GetInfo();
        std::cerr << "first valid frame timeout, frame_errors="
                  << info.frame_error_count << '\n';
        return 1;
    }

    while (true)
    {
        if (auto sample = peripheral.GetSample(); sample.has_value())
        {
            std::cout << *sample << '\n';
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
