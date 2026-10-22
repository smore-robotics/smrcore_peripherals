#pragma once
#include <atomic>
#include <array>
#include <cstdint>
#include <string>

namespace smrcore::peripherals
{
struct PicoTeleopOptions
{
    uint16_t port = 8765;
    bool enable_motion = false;
    bool enable_rotation = true;
    bool head_yaw_clutched = true;
    bool operator_facing_robot = false;
    double translation_scale = 0.8;
    double rotation_scale = 1.0;

    double j6_thumbstick_speed = 0.8;
    uint16_t gripper_force = 80;
    uint16_t gripper_speed = 100;
    std::string robot_ip;
};

struct PicoTeleopStatus
{
    bool running = false;
    int exit_code = -1;
    // Latest observed values; zero before the control loop, not a coherent RT
    // snapshot.
    std::array<double, 6> left_positions{};
    std::array<double, 6> right_positions{};
    double left_condition = 0, right_condition = 0;
    double left_scale = 0, right_scale = 0;
};
// Single-use session. Run blocks; RequestStop is safe from another thread.
// Join the Run caller before destroying this object. No Python callbacks on RT
// path.
class PicoTeleop
{
public:
    PicoTeleop()
    {
        for (auto &v : diagnostics_)
            v.store(0);
    }
    PicoTeleop(const PicoTeleop &) = delete;
    PicoTeleop &operator=(const PicoTeleop &) = delete;
    PicoTeleopStatus GetStatus() const noexcept;
    int Run(const PicoTeleopOptions &options);
    void RequestStop() noexcept { running_.store(false); }
    bool IsRunning() const noexcept { return active_.load(); }
    int GetExitCode() const noexcept { return exit_code_.load(); }

private:
    std::array<std::atomic<double>, 16> diagnostics_;
    std::atomic<bool> started_{false};
    std::atomic<bool> running_{true};
    std::atomic<bool> active_{false};
    std::atomic<int> exit_code_{-1};
};
} // namespace smrcore::peripherals
