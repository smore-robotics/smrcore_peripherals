/**
 * @file app_peripherals_ft_sensor_calib.cpp
 * @brief 外置六维力传感器静态标定（bias / 重力 / 质心 / 安装 yaw）
 *
 * 本应用是纯 SDK 客户端：用 MoveJ 走到多个静态姿态，通过
 * `Calib().GetFtSensorRawState()` 采集 raw wrench 均值，再调用
 * Preview/SaveFtCalibration。串口读数需先由 `app_peripherals_bridge
 * --ft-sensor` 注入控制器。
 *
 * 辨识内容（不是单纯坐标变换）：
 * - force_bias / torque_bias：力/力矩零偏
 * - gravity_n：传感器远端固定质量的重力大小 [N]
 * - com_in_sensor_m：该质量在传感器坐标系下的质心 [m]
 * - yaw_correction_rad：相对名义安装的偏航修正 [rad]
 * - sensor_in_flange_nominal：法兰→传感器名义安装位姿（CLI `--nominal`，默认单位姿）
 *
 * 运行时控制器用上述参数：去零偏 → 减重力 → 变换到当前 tool 原点、Base 表达外力。
 * 这与 `app_sdk_payload_calib`（动力学末端负载辨识）是不同链路。
 *
 * 使用方式：
 *   app_peripherals_bridge --robot <ip> --ft-sensor
 *   app_peripherals_ft_sensor_calib [--robot-ip <ip>]           # Preview
 *   app_peripherals_ft_sensor_calib [--robot-ip <ip>] --save    # 持久化并激活
 *
 * 构建要求：仅在 `SMR_PERIPHERAL_WITH_SDK=ON`（`--with-sdk ON`）时编译。
 * 参考系须为 base；当前 tool 须为 flange，或法兰下零偏置子坐标系。
 */

#include "sdk/robot.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kPoseZeroTolerance = 1e-12;
constexpr int kPostEnableMoveJDelayMs = 200;

std::atomic<bool> g_running{true};

void SignalHandler(int) { g_running.store(false); }

struct AppConfig
{
    std::string robot_ip;
    int settle_ms = 2000;
    int sample_ms = 1000;
    double velocity_ratio = 0.20;
    double joint0_deg = 0.0;
    double joint5_deg = 0.0;
    bool save = false;
    rcore::sdk::Pose sensor_in_flange_nominal;
};

std::vector<std::array<double, 6>>
CalibrationPointsDeg(double joint0_deg = 0.0, double joint5_deg = 0.0)
{
    auto points = std::vector<std::array<double, 6>>{
        {0.0, -90.0, -90.0, -90.0, 60.0, 0.0},
        {0.0, -90.0, -90.0, -90.0, 90.0, 0.0},
        {0.0, -90.0, -90.0, -90.0, 120.0, 0.0},
        {0.0, -90.0, -90.0, -45.0, 120.0, 0.0},
        {0.0, -90.0, -90.0, -45.0, 90.0, 0.0},
        {0.0, -90.0, -90.0, -45.0, 60.0, 0.0},
        {0.0, -90.0, -90.0, -135.0, 60.0, 0.0},
        {0.0, -90.0, -90.0, -135.0, 90.0, 0.0},
        {0.0, -90.0, -90.0, -135.0, 120.0, 0.0}};
    for (auto &point : points)
    {
        point[0] = joint0_deg;
        point[5] = joint5_deg;
    }
    return points;
}

std::array<double, 6> HomePointDeg(double joint0_deg = 0.0,
                                   double joint5_deg = 0.0)
{
    auto point = std::array<double, 6>{0.0, -90.0, -90.0, -90.0, 90.0, 0.0};
    point[0] = joint0_deg;
    point[5] = joint5_deg;
    return point;
}

rcore::sdk::JointPositions ToJointPositionsDeg(const std::array<double, 6> &q)
{
    rcore::sdk::JointPositions positions;
    for (std::size_t i = 0; i < q.size(); ++i)
    {
        positions[i] = q[i] * kPi / 180.0;
    }
    return positions;
}

rcore::sdk::JointPositions
ToJointPositions(const std::array<double, rcore::kNumJoints> &q)
{
    rcore::sdk::JointPositions positions;
    for (std::size_t i = 0; i < q.size(); ++i)
    {
        positions[i] = q[i];
    }
    return positions;
}

std::array<double, 6> UniformVelocityPercentage(double percentage)
{
    std::array<double, 6> velocity{};
    velocity.fill(percentage);
    return velocity;
}

std::vector<std::string> Split(const std::string &text, char delim)
{
    std::stringstream stream(text);
    std::vector<std::string> parts;
    std::string part;
    while (std::getline(stream, part, delim))
    {
        parts.push_back(part);
    }
    return parts;
}

bool ParsePose(const std::string &text, rcore::sdk::Pose &pose)
{
    const auto parts = Split(text, ',');
    if (parts.size() != 6)
    {
        return false;
    }
    rcore::sdk::Vec<double, 3> translation;
    rcore::sdk::Vec<double, 3> rotation;
    try
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            translation[i] = std::stod(parts[i]);
            rotation[i] = std::stod(parts[i + 3]);
        }
    }
    catch (...)
    {
        return false;
    }
    pose = rcore::sdk::Pose::FromEuler(translation, rotation);
    return true;
}

void PrintUsage(const char *exe)
{
    std::cout
        << "Usage: " << exe << " [options]\n\n"
        << "Options:\n"
        << "  --robot-ip <ip>        SDK remote robot IP (default: local DDS)\n"
        << "  --nominal <6 values>   sensor_in_flange_nominal as "
           "x,y,z,roll,pitch,yaw in m/rad (default: identity)\n"
        << "  --settle-ms <n>        Static settle time after MoveJ (default: "
           "2000)\n"
        << "  --sample-ms <n>        Raw averaging window (default: 1000)\n"
        << "  --velocity-ratio <v>   MoveJ speed ratio in (0,1] (default: "
           "0.20)\n"
        << "  --joint0-deg <deg>     Joint 1 angle for all calibration and "
           "home points (default: 0)\n"
        << "  --joint5-deg <deg>     Joint 6 angle for all calibration and "
           "home points (default: 0)\n"
        << "  --save                 Save passing calibration and activate it; "
           "default is Preview only\n"
        << "  --help                 Show this help\n\n"
        << "The current reference frame must be base. The current tool must be "
           "flange, or a zero-offset frame directly under flange.\n"
        << "Run app_peripherals_bridge --ft-sensor first so raw F/T samples "
           "are available via Calib().GetFtSensorRawState().\n";
}

bool ParseArgs(int argc, char *argv[], AppConfig &config, bool &show_help)
{
    show_help = false;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        auto next = [&]() -> const char *
        {
            if (i + 1 >= argc)
            {
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--help")
        {
            show_help = true;
            return true;
        }
        if (arg == "--save")
        {
            config.save = true;
        }
        else if (arg == "--robot-ip")
        {
            const char *value = next();
            if (value == nullptr)
            {
                return false;
            }
            config.robot_ip = value;
        }
        else if (arg == "--nominal")
        {
            const char *value = next();
            if (value == nullptr ||
                !ParsePose(value, config.sensor_in_flange_nominal))
            {
                return false;
            }
        }
        else if (arg == "--settle-ms")
        {
            const char *value = next();
            if (value == nullptr)
            {
                return false;
            }
            config.settle_ms = std::stoi(value);
        }
        else if (arg == "--sample-ms")
        {
            const char *value = next();
            if (value == nullptr)
            {
                return false;
            }
            config.sample_ms = std::stoi(value);
        }
        else if (arg == "--velocity-ratio")
        {
            const char *value = next();
            if (value == nullptr)
            {
                return false;
            }
            config.velocity_ratio = std::stod(value);
        }
        else if (arg == "--joint0-deg")
        {
            const char *value = next();
            if (value == nullptr)
            {
                return false;
            }
            config.joint0_deg = std::stod(value);
        }
        else if (arg == "--joint5-deg")
        {
            const char *value = next();
            if (value == nullptr)
            {
                return false;
            }
            config.joint5_deg = std::stod(value);
        }
        else
        {
            std::cerr << "Unknown option: " << arg << std::endl;
            return false;
        }
    }
    return config.settle_ms >= 0 && config.sample_ms > 0 &&
           std::isfinite(config.velocity_ratio) &&
           config.velocity_ratio > 0.0 && config.velocity_ratio <= 1.0 &&
           std::isfinite(config.joint0_deg) &&
           std::isfinite(config.joint5_deg);
}

class RawWrenchSampler
{
public:
    explicit RawWrenchSampler(rcore::sdk::Robot &robot) : robot_(robot) {}

    bool WaitForSample(int timeout_ms)
    {
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(timeout_ms);
        while (g_running.load() && std::chrono::steady_clock::now() < deadline)
        {
            rcore::sdk::FtSensorRawState state;
            auto result = robot_.Calib().GetFtSensorRawState(state);
            if (result.IsSuccess() && state.valid)
            {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }

    bool CollectMean(int duration_ms, std::array<double, 6> &mean,
                     std::size_t &sample_count)
    {
        std::array<double, 6> sum{};
        std::size_t count = 0;
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(duration_ms);
        while (g_running.load() && std::chrono::steady_clock::now() < deadline)
        {
            rcore::sdk::FtSensorRawState state;
            auto result = robot_.Calib().GetFtSensorRawState(state);
            if (result.IsSuccess() && state.valid)
            {
                for (std::size_t i = 0; i < sum.size(); ++i)
                {
                    sum[i] += state.wrench[i];
                }
                ++count;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        sample_count = count;
        if (count == 0 || !g_running.load())
        {
            return false;
        }
        for (std::size_t i = 0; i < mean.size(); ++i)
        {
            mean[i] = sum[i] / static_cast<double>(count);
        }
        return true;
    }

private:
    rcore::sdk::Robot &robot_;
};

bool UsesBaseFlangePoseContract(const rcore::ControllerConfig &config)
{
    if (config.transform_tree.reference_frame != "base")
    {
        return false;
    }
    if (config.transform_tree.current_tool_frame == "flange")
    {
        return true;
    }
    for (const auto &frame : config.transform_tree.nodes)
    {
        if (frame.name != config.transform_tree.current_tool_frame ||
            frame.parent != "flange")
        {
            continue;
        }
        for (std::size_t i = 0; i < frame.xyz.size(); ++i)
        {
            if (std::abs(frame.xyz[i]) > kPoseZeroTolerance ||
                std::abs(frame.rpy[i]) > kPoseZeroTolerance)
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool SleepWhileRunning(int milliseconds)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(milliseconds);
    while (g_running.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return g_running.load();
}

bool WaitForMotorReady(rcore::sdk::Robot &robot, int timeout_ms)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeout_ms);
    while (g_running.load() && std::chrono::steady_clock::now() < deadline)
    {
        const auto status = robot.GetMotorStatus();
        if (status.enabled && status.operational && !status.estop &&
            !status.error)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

void PrintCalibrationResult(const rcore::sdk::FtSensorCalibrationResult &result)
{
    const auto &p = result.params;
    std::cout << std::setprecision(10) << "force_bias [N]: " << p.force_bias[0]
              << ", " << p.force_bias[1] << ", " << p.force_bias[2] << "\n"
              << "torque_bias [Nm]: " << p.torque_bias[0] << ", "
              << p.torque_bias[1] << ", " << p.torque_bias[2] << "\n"
              << "gravity_n [N]: " << p.gravity_n << "\n"
              << "com_in_sensor_m [m]: " << p.com_in_sensor_m[0] << ", "
              << p.com_in_sensor_m[1] << ", " << p.com_in_sensor_m[2] << "\n"
              << "yaw_correction_rad: " << p.yaw_correction_rad << "\n"
              << "force_rms_n: " << result.force_rms_n
              << ", torque_rms_nm: " << result.torque_rms_nm << std::endl;
}

int Run(const AppConfig &config)
{
    rcore::sdk::Robot robot;
    if (!robot.Initialize(config.robot_ip))
    {
        std::cerr << "Robot SDK initialize failed." << std::endl;
        return 1;
    }

    RawWrenchSampler sampler(robot);
    const auto controller_config = robot.Config().GetControllerConfig();
    if (!UsesBaseFlangePoseContract(controller_config))
    {
        std::cerr << "Calibration requires reference_frame=base and a flange "
                     "or zero-offset flange tool frame; current selection is "
                  << controller_config.transform_tree.reference_frame << "/"
                  << controller_config.transform_tree.current_tool_frame << "."
                  << std::endl;
        robot.Shutdown();
        return 1;
    }

    const auto initial_motor_status = robot.GetMotorStatus();
    const bool initially_enabled = initial_motor_status.enabled;
    bool enabled_by_app = false;
    bool sensor_started = false;
    bool moved = false;

    auto cleanup = [&]()
    {
        if (moved)
        {
            std::cout << "Returning to configured home point..." << std::endl;
            const auto home_point = ToJointPositionsDeg(
                HomePointDeg(config.joint0_deg, config.joint5_deg));
            auto home_result = robot.MoveJ(home_point, false);
            if (home_result.IsFailure())
            {
                std::cerr << "Return home failed: " << home_result.GetErrorMsg()
                          << std::endl;
            }
        }
        if (sensor_started)
        {
            auto release = robot.Calib().ReleaseFtSensor();
            if (release.IsFailure())
            {
                std::cerr << "ReleaseFtSensor failed: " << release.GetErrorMsg()
                          << std::endl;
            }
        }
        if (enabled_by_app)
        {
            (void)robot.Disable();
        }
        robot.Shutdown();
    };

    auto sensor_result = robot.Calib().EnsureFtSensor();
    if (sensor_result.IsFailure())
    {
        std::cerr << "EnsureFtSensor failed (" << sensor_result.GetErrorCode()
                  << "): " << sensor_result.GetErrorMsg() << std::endl;
        cleanup();
        return 1;
    }
    sensor_started = true;
    if (!sampler.WaitForSample(1500))
    {
        std::cerr << "No valid F/T raw sample after EnsureFtSensor; start "
                     "app_peripherals_bridge --ft-sensor first."
                  << std::endl;
        cleanup();
        return 1;
    }

    if (!initially_enabled)
    {
        std::cout << "Enabling robot motors..." << std::endl;
        auto enable = robot.Enable();
        if (enable.IsFailure())
        {
            std::cerr << "Enable failed: " << enable.GetErrorMsg() << std::endl;
            cleanup();
            return 1;
        }
        enabled_by_app = true;
    }
    if (!WaitForMotorReady(robot, 3000))
    {
        const auto status = robot.GetMotorStatus();
        std::cerr << "Motors not ready for calibration MoveJ: enabled="
                  << status.enabled << ", operational=" << status.operational
                  << ", estop=" << status.estop << ", error=" << status.error
                  << "." << std::endl;
        cleanup();
        return 1;
    }
    if (!SleepWhileRunning(kPostEnableMoveJDelayMs))
    {
        std::cerr << "Calibration interrupted before first MoveJ." << std::endl;
        cleanup();
        return 1;
    }
    auto speed = robot.Config().SetVelocityPercentage(
        UniformVelocityPercentage(config.velocity_ratio));
    if (speed.IsFailure())
    {
        std::cerr << "SetVelocityPercentage failed: " << speed.GetErrorMsg()
                  << std::endl;
        cleanup();
        return 1;
    }

    rcore::sdk::FtSensorCalibrationRequest request;
    request.sensor_in_flange_nominal = config.sensor_in_flange_nominal;
    const auto points =
        CalibrationPointsDeg(config.joint0_deg, config.joint5_deg);
    request.samples.reserve(points.size());

    std::cout << "Collecting " << points.size()
              << " static samples; mode=" << (config.save ? "save" : "preview")
              << "." << std::endl;
    for (std::size_t i = 0; i < points.size() && g_running.load(); ++i)
    {
        std::cout << "Point " << (i + 1) << "/" << points.size() << ": MoveJ"
                  << std::endl;
        auto move = robot.MoveJ(ToJointPositionsDeg(points[i]), false);
        if (move.IsFailure())
        {
            std::cerr << "MoveJ failed: " << move.GetErrorMsg() << std::endl;
            cleanup();
            return 1;
        }
        moved = true;
        if (!SleepWhileRunning(config.settle_ms))
        {
            break;
        }

        rcore::sdk::FtSensorCalibrationSample sample;
        sample.id = "pose-" + std::to_string(i + 1);
        const auto state = robot.GetState();
        auto fk = robot.Motion().ForwardKinematics(
            ToJointPositions(state.positions), sample.flange_in_base);
        if (fk.IsFailure())
        {
            std::cerr << "ForwardKinematics failed: " << fk.GetErrorMsg()
                      << std::endl;
            cleanup();
            return 1;
        }
        std::size_t sample_count = 0;
        if (!sampler.CollectMean(config.sample_ms, sample.raw_wrench,
                                 sample_count))
        {
            std::cerr << "Failed to acquire static raw wrench window."
                      << std::endl;
            cleanup();
            return 1;
        }
        std::cout << "  raw samples: " << sample_count
                  << ", mean force [N]: " << sample.raw_wrench[0] << ", "
                  << sample.raw_wrench[1] << ", " << sample.raw_wrench[2]
                  << std::endl;
        request.samples.push_back(sample);
    }
    if (!g_running.load())
    {
        std::cerr << "Calibration interrupted." << std::endl;
        cleanup();
        return 1;
    }

    const auto preview = robot.Calib().PreviewFtCalibration(request);
    if (!preview.status.success)
    {
        std::cerr << "PreviewFtSensorCalibration failed ("
                  << preview.status.error_code
                  << "): " << preview.status.error_msg << std::endl;
        cleanup();
        return 1;
    }
    std::cout << "Preview result:" << std::endl;
    PrintCalibrationResult(preview);

    if (config.save)
    {
        const auto saved = robot.Calib().SaveFtCalibration(request);
        if (!saved.status.success)
        {
            std::cerr << "SaveFtSensorCalibration failed ("
                      << saved.status.error_code
                      << "): " << saved.status.error_msg << std::endl;
            cleanup();
            return 1;
        }
        std::cout << "Saved and activated calibration parameters." << std::endl;
        PrintCalibrationResult(saved);
    }

    cleanup();
    return 0;
}

} // namespace

int main(int argc, char *argv[])
{
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    AppConfig config;
    bool show_help = false;
    try
    {
        if (!ParseArgs(argc, argv, config, show_help))
        {
            PrintUsage(argv[0]);
            return 1;
        }
    }
    catch (const std::exception &exception)
    {
        std::cerr << "Invalid argument: " << exception.what() << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }
    if (show_help)
    {
        PrintUsage(argv[0]);
        return 0;
    }
    return Run(config);
}
