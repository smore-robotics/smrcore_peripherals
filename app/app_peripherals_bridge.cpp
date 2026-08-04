/**
 * @file app_peripherals_bridge.cpp
 * @brief 外设统一 SDK bridge 应用
 *
 * 本应用在一个进程内只创建一个机器人 SDK 会话，然后按需启动
 * SpaceMouse 与/或六维力/力矩传感器 reader，并通过
 * `Robot::Peripheral().UpdateSpaceMouseSample()` /
 * `UpdateFtSensorSample()` 将采样送入机器人控制器。
 *
 * 使用方式：
 * - FDCC 同时需要 SpaceMouse 与力传感器：
 *   `app_peripherals_bridge --robot <robot_ip>`
 * - 只启用 SpaceMouse：
 *   `app_peripherals_bridge --robot <robot_ip> --spacemouse`
 * - 只启用力传感器：
 *   `app_peripherals_bridge --robot <robot_ip> --ft-sensor`
 * - 同时显式启用两者：
 *   `app_peripherals_bridge --robot <robot_ip> --spacemouse --ft-sensor`
 *
 * 选择规则：不传 `--spacemouse`/`--ft-sensor` 时默认两路都启动；只要出现
 * 任一选择参数，则仅启动显式选择的外设。设备参数只影响对应外设，例如
 * `--spacemouse-device /dev/input/event7`、`--ft-serial-port /dev/ttyUSB0`。
 *
 * 构建要求：仅在 `SMR_PERIPHERAL_WITH_SDK=ON`（`--with-sdk ON`）时编译本目标。
 */

#include "peripherals/peripherals.hpp"
#include "sdk/robot.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace
{

std::atomic<bool> g_running{true};

void HandleSignal(int)
{
    g_running.store(false, std::memory_order_release);
}

struct BridgeOptions
{
    std::string robot_ip;
    bool spacemouse_enabled{true};
    bool ft_sensor_enabled{true};
    bool help_requested{false};
    smrcore::peripherals::SpaceMouseOptions spacemouse;
    smrcore::peripherals::FtSensorOptions ft_sensor;
};

void PrintUsage(const char *program)
{
    std::cerr
        << "usage: " << program << " [--robot IP] "
        << "[--spacemouse] [--ft-sensor]\n"
        << "       [--spacemouse-device PATH] "
           "[--spacemouse-sample-rate HZ]\n"
        << "       [--ft-serial-port PATH] [--ft-sensor-type TYPE] "
           "[--ft-baud-rate BAUD]\n"
        << "       [--ft-sample-rate HZ] [--ft-stale-timeout-ms MS]\n";
}

bool ParseCli(int argc, char **argv, BridgeOptions &options)
{
    bool explicit_enable = false;
    options.ft_sensor.serial_port = "/dev/ttyUSB0";
    options.ft_sensor.sample_rate_hz = 1000.0;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--robot" && i + 1 < argc)
        {
            options.robot_ip = argv[++i];
        }
        else if (arg == "--spacemouse")
        {
            if (!explicit_enable)
            {
                options.spacemouse_enabled = false;
                options.ft_sensor_enabled = false;
                explicit_enable = true;
            }
            options.spacemouse_enabled = true;
        }
        else if (arg == "--ft-sensor")
        {
            if (!explicit_enable)
            {
                options.spacemouse_enabled = false;
                options.ft_sensor_enabled = false;
                explicit_enable = true;
            }
            options.ft_sensor_enabled = true;
        }
        else if (arg == "--spacemouse-device" && i + 1 < argc)
        {
            options.spacemouse.device_path = argv[++i];
        }
        else if (arg == "--spacemouse-sample-rate" && i + 1 < argc)
        {
            options.spacemouse.sample_rate_hz = std::atof(argv[++i]);
        }
        else if (arg == "--ft-serial-port" && i + 1 < argc)
        {
            options.ft_sensor.serial_port = argv[++i];
        }
        else if (arg == "--ft-sensor-type" && i + 1 < argc)
        {
            options.ft_sensor.sensor_type = argv[++i];
        }
        else if (arg == "--ft-baud-rate" && i + 1 < argc)
        {
            options.ft_sensor.baud_rate = std::atoi(argv[++i]);
        }
        else if (arg == "--ft-sample-rate" && i + 1 < argc)
        {
            options.ft_sensor.sample_rate_hz = std::atof(argv[++i]);
        }
        else if (arg == "--ft-stale-timeout-ms" && i + 1 < argc)
        {
            options.ft_sensor.stale_timeout_ms = std::atoi(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h")
        {
            PrintUsage(argv[0]);
            options.help_requested = true;
            return true;
        }
        else
        {
            std::cerr << "unknown or incomplete argument: " << arg << '\n';
            PrintUsage(argv[0]);
            return false;
        }
    }

    return true;
}

rcore::GripperCommand ToSdk(smrcore::peripherals::GripperCommand command)
{
    return command == smrcore::peripherals::GripperCommand::Close
               ? rcore::GripperCommand::Close
               : rcore::GripperCommand::Open;
}

rcore::SpaceMouseSample ToSdk(
    const smrcore::peripherals::SpaceMouseSample &sample)
{
    rcore::SpaceMouseSample out;
    out.x = sample.x;
    out.y = sample.y;
    out.z = sample.z;
    out.roll = sample.roll;
    out.pitch = sample.pitch;
    out.yaw = sample.yaw;
    out.gripper_command = ToSdk(sample.gripper_command);
    out.timestamp_sec = sample.timestamp_sec;
    return out;
}

rcore::FtSensorSample ToSdk(const smrcore::peripherals::FtSensorSample &sample)
{
    rcore::FtSensorSample out;
    out.fx = sample.fx;
    out.fy = sample.fy;
    out.fz = sample.fz;
    out.tx = sample.tx;
    out.ty = sample.ty;
    out.tz = sample.tz;
    out.timestamp_sec = sample.timestamp_sec;
    return out;
}

void RunSpaceMouseBridge(smrcore::peripherals::SpaceMouse &spacemouse,
                         rcore::sdk::PeripheralApi &peripheral,
                         std::mutex &sdk_mutex)
{
    while (g_running.load(std::memory_order_acquire))
    {
        if (auto sample = spacemouse.GetSample())
        {
            std::lock_guard<std::mutex> lock(sdk_mutex);
            auto result = peripheral.UpdateSpaceMouseSample(ToSdk(*sample));
            if (!result.IsSuccess())
            {
                std::cerr << "UpdateSpaceMouseSample failed: "
                          << result.GetErrorCode() << " "
                          << result.GetErrorMsg() << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

void RunFtSensorBridge(smrcore::peripherals::FtSensor &sensor,
                       rcore::sdk::PeripheralApi &peripheral,
                       std::mutex &sdk_mutex)
{
    while (g_running.load(std::memory_order_acquire))
    {
        if (auto sample = sensor.GetSample())
        {
            std::lock_guard<std::mutex> lock(sdk_mutex);
            auto result = peripheral.UpdateFtSensorSample(ToSdk(*sample));
            if (!result.IsSuccess())
            {
                std::cerr << "UpdateFtSensorSample failed: "
                          << result.GetErrorCode() << " "
                          << result.GetErrorMsg() << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace

int main(int argc, char **argv)
{
    BridgeOptions options;
    if (!ParseCli(argc, argv, options))
    {
        return 1;
    }
    if (options.help_requested)
    {
        return 0;
    }

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    rcore::sdk::Robot robot;
    std::cout << "正在连接机器人"
              << (options.robot_ip.empty() ? "" : " " + options.robot_ip)
              << " ..." << std::endl;
    if (!robot.Initialize(options.robot_ip))
    {
        std::cerr << "failed to initialize robot SDK" << std::endl;
        return 1;
    }
    std::cout << "机器人 SDK 连接成功" << std::endl;

    smrcore::peripherals::SpaceMouse spacemouse;
    if (options.spacemouse_enabled)
    {
        std::cout << "正在启动 SpaceMouse ..." << std::endl;
        if (!spacemouse.Initialize(options.spacemouse) || !spacemouse.Start())
        {
            std::cerr << "failed to start SpaceMouse reader" << std::endl;
            return 1;
        }
        std::cout << "SpaceMouse 已启动" << std::endl;
    }

    smrcore::peripherals::FtSensor sensor;
    if (options.ft_sensor_enabled)
    {
        std::cout << "正在启动力传感器 " << options.ft_sensor.serial_port
                  << " ..." << std::endl;
        if (!sensor.Initialize(options.ft_sensor) || !sensor.Start())
        {
            std::cerr << "failed to start F/T sensor reader" << std::endl;
            return 1;
        }
        if (!sensor.WaitForFirstSample())
        {
            const auto info = sensor.GetInfo();
            std::cerr << "first valid F/T frame timeout, frame_errors="
                      << info.frame_error_count << std::endl;
            return 1;
        }
        std::cout << "力传感器已启动" << std::endl;
    }

    auto peripheral = robot.Peripheral();
    std::mutex sdk_mutex;
    std::thread spacemouse_thread;
    std::thread ft_sensor_thread;

    if (options.spacemouse_enabled)
    {
        spacemouse_thread = std::thread(RunSpaceMouseBridge,
                                        std::ref(spacemouse),
                                        std::ref(peripheral),
                                        std::ref(sdk_mutex));
    }
    if (options.ft_sensor_enabled)
    {
        ft_sensor_thread = std::thread(RunFtSensorBridge, std::ref(sensor),
                                       std::ref(peripheral),
                                       std::ref(sdk_mutex));
    }

    std::cout << "连接成功，正在发送外设消息"
              << (options.spacemouse_enabled ? " [SpaceMouse]" : "")
              << (options.ft_sensor_enabled ? " [F/T]" : "")
              << "（Ctrl+C 退出）" << std::endl;

    if (spacemouse_thread.joinable())
    {
        spacemouse_thread.join();
    }
    if (ft_sensor_thread.joinable())
    {
        ft_sensor_thread.join();
    }
    std::cout << "外设 bridge 已停止" << std::endl;
    return 0;
}
