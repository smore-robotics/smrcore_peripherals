// Included inside the teleop implementation namespace for the comparison build.
// Restore the historical fixed feedback hold, not the estimated forward target.
// Keep bounded fresh-feedback checks and cancellation ownership from the new core.
class PicoLegacyReleaseStopper : public PicoServoStopper
{
public:
    void Request(rcore::sdk::Robot &robot, bool joint_mode)
    {
        if (pending_.valid()) return;
        pending_ = std::async(std::launch::async, [&robot, joint_mode]() -> std::string
        {
            try
            {
                const auto initial = robot.GetState();
                rcore::sdk::JointPositions hold{};
                rcore::sdk::Pose pose;
                if (!std::isfinite(initial.timestamp) || initial.timestamp <= 0)
                    return "invalid stop timestamp";
                for (size_t i = 0; i < 6; ++i)
                {
                    if (!std::isfinite(initial.positions[i]))
                        return "invalid stop joint position";
                    hold[i] = initial.positions[i];
                }
                if (!joint_mode)
                {
                    if (!initial.cartesian_valid) return "invalid stop TCP";
                    for (size_t i = 0; i < 3; ++i)
                    {
                        if (!std::isfinite(initial.cartesian_position[i]) ||
                            !std::isfinite(initial.cartesian_orientation[i]))
                            return "nonfinite stop TCP";
                        pose.tvec()[i] = initial.cartesian_position[i];
                        pose.rvec()[i] = initial.cartesian_orientation[i];
                    }
                }
                auto last_timestamp = initial.timestamp;
                auto fresh_at = std::chrono::steady_clock::now();
                const auto deadline = fresh_at + std::chrono::seconds(3);
                auto still_since = std::chrono::steady_clock::time_point{};
                size_t sent = 0;
                while (std::chrono::steady_clock::now() < deadline)
                {
                    const auto now = std::chrono::steady_clock::now();
                    const auto state = robot.GetState();
                    if (now - fresh_at > std::chrono::milliseconds(100))
                        return "stale feedback during legacy stop";
                    if (state.errors.GetLatchedErrorCount() > 0)
                        return "controller fault during legacy stop";
                    if (std::isfinite(state.timestamp) && state.timestamp > last_timestamp)
                    {
                        last_timestamp = state.timestamp;
                        fresh_at = now;
                        bool still = sent >= 100;
                        for (size_t i = 0; i < 6; ++i)
                        {
                            if (!std::isfinite(state.positions[i]) ||
                                !std::isfinite(state.velocities[i]))
                                return "invalid feedback during legacy stop";
                            still = still && std::abs(state.velocities[i]) <= 0.01;
                        }
                        if (!still) still_since = {};
                        else if (still_since == std::chrono::steady_clock::time_point{})
                            still_since = now;
                        else if (now - still_since >= std::chrono::milliseconds(50))
                            return {};
                    }
                    if (sent < 100)
                    {
                        if (joint_mode)
                            robot.Motion().JointPassthrough(hold, rcore::sdk::JointVelocities{});
                        else
                            robot.Motion().ServoP(pose);
                        ++sent;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                return "legacy stop did not reach standstill within 3s";
            }
            catch (const std::exception &error) { return error.what(); }
        });
    }
    bool Pending() const { return pending_.valid(); }
    bool Poll(std::string &error)
    {
        if (!pending_.valid() || pending_.wait_for(std::chrono::seconds(0)) !=
            std::future_status::ready) return false;
        error = pending_.get();
        return true;
    }
    void Wait() { if (pending_.valid()) pending_.wait(); }
private:
    std::future<std::string> pending_;
};
