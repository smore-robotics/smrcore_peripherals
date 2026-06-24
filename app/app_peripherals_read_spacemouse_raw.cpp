#include "peripherals/peripherals.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    smrcore::peripherals::SpaceMouse peripheral;
    smrcore::peripherals::SpaceMouseOptions options;
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
