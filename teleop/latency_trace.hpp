#pragma once

#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// Optional local diagnostic datagrams. Nonblocking; Python owns disk I/O.
class PicoLatencyTrace
{
public:
    PicoLatencyTrace()
    {
        const char *path = std::getenv("RCORE_PICO_LATENCY_SOCKET");
        if (!path || !*path) return;
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        if (std::strlen(path) >= sizeof(address.sun_path)) return;
        std::strcpy(address.sun_path, path);
        fd_ = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (fd_ >= 0 && ::connect(fd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)))
        {
            ::close(fd_);
            fd_ = -1;
        }
    }
    ~PicoLatencyTrace() { if (fd_ >= 0) ::close(fd_); }
    PicoLatencyTrace(const PicoLatencyTrace &) = delete;
    PicoLatencyTrace &operator=(const PicoLatencyTrace &) = delete;

    void Record(int arm, uint64_t sequence, double received,
                const Eigen::Isometry3d &target, const Eigen::Vector3d &rpy,
                const rcore::RobotState &state, bool active, std::array<double, 12> stages = {})
    {
        if (fd_ < 0) return;
        const double now = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        // Preserve the actual first ServoP call for each approved candidate,
        // including candidates first sent between diagnostic sampling ticks.
        if (active && stages[2] > 0)
        {
            if (candidate_[arm] != stages[2])
            {
                candidate_[arm] = stages[2];
                first_start_[arm] = stages[7];
                first_end_[arm] = stages[8];
            }
            stages[5] = first_start_[arm];
            stages[6] = first_end_[arm];
        }
        // 200 Hz diagnostic ceiling, independent of the 1 kHz control loop.
        if (now - last_[arm] < 0.005) return;
        last_[arm] = now;
        std::array<double, 40> row{};
        row[0] = 2; row[1] = arm; row[2] = static_cast<double>(sequence);
        row[3] = received; row[4] = now;
        row[5] = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        row[6] = state.timestamp; row[7] = state.cartesian_valid;
        for (int i = 0; i < 3; ++i)
        {
            row[8+i] = target.translation()[i]; row[11+i] = rpy[i];
            row[14+i] = state.cartesian_position[i];
            row[17+i] = state.cartesian_orientation[i];
        }
        for (int i = 0; i < 6; ++i) row[20+i] = state.velocities[i];
        row[26] = static_cast<double>(dropped_); row[27] = active;
        for (size_t i = 0; i < stages.size(); ++i) row[28+i] = stages[i];
        if (::send(fd_, row.data(), sizeof(row), MSG_DONTWAIT | MSG_NOSIGNAL) != sizeof(row)) ++dropped_;
    }
private:
    int fd_ = -1;
    uint64_t dropped_ = 0;
    std::array<double, 2> last_{}, candidate_{}, first_start_{}, first_end_{};
};
