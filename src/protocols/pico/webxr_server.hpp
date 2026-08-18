/**
 * @file webxr_server.hpp
 * @brief PICO WebXR controller pose receiver.
 */

#pragma once

#include "peripherals/types.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace smrcore::peripherals::protocols::pico
{

using smrcore::peripherals::PicoSample;

class WebXrServer
{
public:
    WebXrServer() = default;
    ~WebXrServer();

    WebXrServer(const WebXrServer &) = delete;
    WebXrServer &operator=(const WebXrServer &) = delete;

    bool Start(uint16_t port, std::string &error);
    void Stop();
    bool IsRunning() const;
    bool IsClientConnected() const;
    std::optional<PicoSample> TakeSample();

private:
    void Run();
    void HandleClient(int client_fd);
    bool ParsePacket(const std::string &text, PicoSample &packet) const;

    mutable std::mutex packet_mutex_;
    std::optional<PicoSample> latest_packet_;
    std::atomic<bool> running_{false};
    std::atomic<bool> client_connected_{false};
    std::atomic<int> listen_fd_{-1};
    std::atomic<int> client_fd_{-1};
    std::thread thread_;
};

} // namespace smrcore::peripherals::protocols::pico
