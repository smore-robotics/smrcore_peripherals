#include "peripherals/peripherals.hpp"
#include "sdk/robot.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace
{

bool ParseCli(int argc, char **argv, std::string &robot_ip,
              smrcore::peripherals::FtSensorOptions &options)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--robot" && i + 1 < argc)
        {
            robot_ip = argv[++i];
        }
        else if (arg == "--serial-port" && i + 1 < argc)
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
    std::string robot_ip;
    smrcore::peripherals::FtSensorOptions options;
    options.serial_port = "/dev/ttyUSB0";
    options.sample_rate_hz = 1000.0;
    if (!ParseCli(argc, argv, robot_ip, options))
    {
        return 1;
    }

    rcore::sdk::Robot robot;
    if (!robot.Initialize(robot_ip))
    {
        std::cerr << "failed to initialize robot SDK" << std::endl;
        return 1;
    }

    smrcore::peripherals::FtSensor sensor;
    if (!sensor.Initialize(options) || !sensor.Start())
    {
        std::cerr << "failed to start F/T sensor reader" << std::endl;
        return 1;
    }
    if (!sensor.WaitForFirstSample())
    {
        const auto info = sensor.GetInfo();
        std::cerr << "first valid frame timeout, frame_errors="
                  << info.frame_error_count << std::endl;
        return 1;
    }

    std::cout << "Streaming external F/T samples for calibration workflow. "
                 "Collect static windows in the client and call "
                 "robot.Calib().PreviewFtCalibration()/SaveFtCalibration()."
              << std::endl;

    auto peripheral = robot.Peripheral();
    while (true)
    {
        if (auto sample = sensor.GetSample())
        {
            rcore::FtSensorSample out;
            out.fx = sample->fx;
            out.fy = sample->fy;
            out.fz = sample->fz;
            out.tx = sample->tx;
            out.ty = sample->ty;
            out.tz = sample->tz;
            out.timestamp_sec = sample->timestamp_sec;
            peripheral.UpdateFtSensorSample(out);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
