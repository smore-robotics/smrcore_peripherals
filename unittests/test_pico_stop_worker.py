"""Offline regression of the production Pico stop worker with fake SDK feedback."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class PicoStopWorkerTest(unittest.TestCase):
    def test_planned_stop_target_and_feedback_gate(self):
        source = (Path(__file__).resolve().parents[1] / 'teleop/pico_teleop.cpp').read_text()
        start = source.index('class PicoStopTarget\n')
        end = source.index('// Preflight', start)
        self.assertNotIn('.JointPassthrough(', source)
        harness = r'''
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <future>
#include <limits>
#include <string>
#include <thread>
namespace rcore {
struct Errors { int count = 0; int GetLatchedErrorCount() const { return count; } };
struct RobotState { double timestamp = 1; std::array<double,6> positions{}, velocities{}; Errors errors; };
struct ControllerConfig {
    std::array<double,6> max_velocity{}, velocity_percentage{}, max_acceleration{}, max_jerk{}, position_limit_min{}, position_limit_max{};
    ControllerConfig() {
        max_velocity.fill(1); velocity_percentage.fill(0.5);
        max_acceleration.fill(10); max_jerk.fill(100);
        position_limit_min.fill(-6); position_limit_max.fill(6);
    }
};
namespace sdk {
using JointPositions = std::array<double,6>;
using JointVelocities = std::array<double,6>;
struct Robot {
    bool stale = false, fault = false, invalid = false, never_settles = false, delayed = false;
    ControllerConfig limits;
    std::atomic<int> calls{0};
    JointPositions last_q{}; JointVelocities last_v{};
    std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
    Robot() {last_q.fill(0.2); last_v.fill(0.2);}
    Robot &Motion() { return *this; }
    Robot &Config() { return *this; }
    ControllerConfig GetControllerConfig() {return limits;}
    // No raw passthrough/StopMotion: targets must use the existing planner.
    void ServoJ(const JointPositions &q) {
        if (calls > 0) for (int i=0;i<6;++i) assert(q[i]==last_q[i]);
        else for (double value:q) assert(value>0.2 && value<0.22);
        last_q=q; ++calls;
        if (delayed) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    RobotState GetState() {
        RobotState state;
        const auto elapsed = std::chrono::steady_clock::now() - began;
        if (!stale) state.timestamp += std::chrono::duration<double>(elapsed).count();
        state.positions=last_q;
        if (calls==0 || elapsed<std::chrono::milliseconds(60)) state.velocities.fill(0.2);
        if (never_settles && calls>0) {state.velocities.fill(0);state.positions[0]+=0.01;}
        if (invalid) state.positions[0] = std::numeric_limits<double>::quiet_NaN();
        state.errors.count = fault ? 1 : 0;
        return state;
    }
};
}}
''' + source[start:end] + r'''
std::string Complete(PicoServoStopper &stopper) {
    std::string error;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    while (std::chrono::steady_clock::now() < deadline) {
        if (stopper.Poll(error)) return error;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(false); return "timeout";
}
int main() {
    // Forward endpoint estimate for positive, negative and stationary axes.
    {
        rcore::ControllerConfig limits; rcore::RobotState state;
        state.velocities = {0.2, -0.8, 0, 1.0, -0.1, 0.001};
        rcore::sdk::JointPositions q{};
        assert(PicoStopTarget::Build(limits,state,q));
        for(int i=0;i<6;++i) {
            assert(q[i]*state.velocities[i]>=0);
            const double speed=std::abs(state.velocities[i]);
            const double duration=std::max(0.02,std::max(1.5*speed/10,std::sqrt(6*speed/100)));
            assert(std::abs(q[i]-state.velocities[i]*duration/2)<1e-12);
        }
        auto saved=q;
        state.positions[5]=6;
        assert(!PicoStopTarget::Build(limits,state,q)); assert(q==saved);
        state.positions[5]=0; limits.max_jerk[0]=0;
        assert(!PicoStopTarget::Build(limits,state,q));
    }
    {
        rcore::sdk::Robot robot; PicoServoStopper stopper;
        assert(stopper.Configure(robot));
        assert(std::abs(stopper.LimitJ6Speed(0.8)-0.4)<1e-12);
        robot.limits.velocity_percentage[5]=0;
        assert(!stopper.Configure(robot));
    }
    {
        rcore::sdk::Robot robot; robot.delayed=true; PicoServoStopper stopper;
        assert(stopper.Configure(robot)); stopper.Request(robot);
        assert(Complete(stopper).empty());
    }

    {
        rcore::sdk::Robot robot; PicoServoStopper stopper;
        const auto began = std::chrono::steady_clock::now();
        assert(stopper.Configure(robot)); stopper.Request(robot); stopper.Request(robot);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::string error;
        assert(!stopper.Poll(error));
        assert(stopper.Pending());
        assert(Complete(stopper).empty());
        assert(std::chrono::steady_clock::now() - began >= std::chrono::milliseconds(110));
        assert(robot.calls > 10);
        assert(!stopper.Pending());
        assert(!stopper.Poll(error));
    }
    {
        rcore::sdk::Robot robot; robot.stale = true; PicoServoStopper stopper;
        assert(stopper.Configure(robot)); stopper.Request(robot); assert(!Complete(stopper).empty());
    }
    for (int mode = 0; mode < 3; ++mode) {
        rcore::sdk::Robot robot;
        robot.fault = mode == 0;
        robot.invalid = mode == 1;
        robot.never_settles = mode == 2;
        PicoServoStopper stopper; assert(stopper.Configure(robot)); stopper.Request(robot);
        assert(!Complete(stopper).empty());
        if (mode < 2) assert(robot.calls == 0);
    }
}
'''
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'test.cpp').write_text(harness)
            subprocess.run(['g++', '-std=c++17', '-pthread', str(root / 'test.cpp'),
                            '-o', str(root / 'test')], check=True)
            subprocess.run([str(root / 'test')], check=True, timeout=10)


if __name__ == '__main__':
    unittest.main()
