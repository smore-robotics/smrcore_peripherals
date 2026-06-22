#include "peripherals/peripherals.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace
{

bool ParseOptions(int argc, char **argv, smrcore::peripherals::SpaceMouseOptions &options)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--device" && i + 1 < argc)
        {
            options.device_path = argv[++i];
        }
        else if (arg == "--sample-rate" && i + 1 < argc)
        {
            options.sample_rate_hz = std::atof(argv[++i]);
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
    smrcore::peripherals::SpaceMouseOptions options;
    if (!ParseOptions(argc, argv, options))
    {
        return 1;
    }

    smrcore::peripherals::SpaceMouse peripheral;
    if (!peripheral.Initialize(options) || !peripheral.Start())
    {
        std::cerr << "failed to start SpaceMouse peripheral\n";
        return 1;
    }

    while (true)
    {
        if (auto sample = peripheral.GetRawSample())
        {
            std::cout << *sample << '\n';
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
