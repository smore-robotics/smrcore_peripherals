/**
 * @file app_peripherals_read_pico.cpp
 * @brief 无机器人依赖的 PICO WebXR 读数工具
 */

#include "peripherals/pico/pico.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

namespace
{

std::atomic<bool> g_running{true};

void HandleSignal(int) { g_running.store(false, std::memory_order_release); }

void PrintUsage(const char *program)
{
    std::cout << "用法: " << program << " [--port 8765]\n";
}

void PrintPosition(const std::array<double, 3> &position, bool tracked)
{
    if (!tracked)
    {
        std::cout << " xyz[m]=N/A";
        return;
    }
    std::cout << " xyz[m]=(" << std::fixed << std::setprecision(4)
              << position[0] << "," << position[1] << "," << position[2] << ")";
}

bool ParsePort(const std::string &text, uint16_t &port)
{
    try
    {
        std::size_t parsed = 0;
        const unsigned long value = std::stoul(text, &parsed);
        if (parsed != text.size() || value == 0 || value > 65535)
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

} // namespace

int main(int argc, char **argv)
{
    smrcore::peripherals::PicoOptions options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--port" && i + 1 < argc)
        {
            if (!ParsePort(argv[++i], options.port))
            {
                PrintUsage(argv[0]);
                return 2;
            }
        }
        else if (argument == "--help" || argument == "-h")
        {
            PrintUsage(argv[0]);
            return 0;
        }
        else
        {
            PrintUsage(argv[0]);
            return 2;
        }
    }

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    smrcore::peripherals::Pico pico;
    if (!pico.Initialize(options) || !pico.Start())
    {
        std::cerr << "PICO WebXR 服务启动失败: " << pico.GetLastError() << '\n';
        return 1;
    }

    std::cout << "PICO 外设服务: " << pico.GetInfo().path << '\n'
              << "USB 映射: adb reverse tcp:" << options.port
              << " tcp:" << options.port << '\n'
              << "等待 PICO WebXR 数据，Ctrl-C 退出..." << std::endl;

    while (g_running.load(std::memory_order_acquire))
    {
        if (auto sample = pico.GetSample())
        {
            std::cout << "seq=" << sample->sequence
                      << " ref=" << sample->reference_space << " headset="
                      << (sample->headset.tracked ? "tracked" : "missing");
            PrintPosition(sample->headset.position, sample->headset.tracked);
            std::cout << " L=" << (sample->left.tracked ? "tracked" : "missing");
            PrintPosition(sample->left.position, sample->left.tracked);
            std::cout << " grip=" << std::fixed << std::setprecision(2)
                      << sample->left.grip << " trigger=" << sample->left.trigger
                      << " R=" << (sample->right.tracked ? "tracked" : "missing");
            PrintPosition(sample->right.position, sample->right.tracked);
            std::cout << " grip=" << std::fixed << std::setprecision(2)
                      << sample->right.grip << " trigger=" << sample->right.trigger
                      << '\n';
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    pico.Stop();
    return 0;
}
