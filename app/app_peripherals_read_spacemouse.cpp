#include "peripherals/peripherals.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    smrcore::peripherals::SpaceMouse peripheral;
    smrcore::peripherals::SpaceMouseOptions options;
    options.sample_rate_hz = 125.0;
    if (!peripheral.Initialize(options) || !peripheral.Start())
    {
        std::cerr << "failed to start SpaceMouse peripheral\n";
        return 1;
    }

    while (true)
    {
        if (auto sample = peripheral.GetSample(); sample.has_value())
        {
            std::cout << *sample << '\n';
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
