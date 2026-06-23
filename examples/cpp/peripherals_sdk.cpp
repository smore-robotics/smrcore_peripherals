/**
 * @file peripherals_sdk.cpp
 * @brief 外设采样经 Robot SDK 注入控制器（SpaceMouse + 六维力传感器统一示例）
 *
 * FDCC 柔顺控制需要同时向机器人提供 SpaceMouse 遥操作输入与外部六维力/力矩
 * 传感器数据。本示例在一个进程内只创建一个 `rcore::sdk::Robot` 会话，默认
 * 同时启动两路外设 reader，并通过
 * `Robot::Peripheral().UpdateSpaceMouseSample()` /
 * `UpdateFtSensorSample()` 把采样写入 rcore 控制器使用的 DDS topic。
 *
 * 设备路径、串口、波特率等参数在下方「设备配置」区按现场修改；命令行仅保留
 * 机器人连接与「只开一路外设」的开关，避免 FDCC 用户误开两个进程。
 *
 * 使用方式：
 * - FDCC（默认两路都开）：
 *   `./build/bin/peripherals_sdk`
 *   `./build/bin/peripherals_sdk --robot <robot_ip>`
 * - 只开 SpaceMouse（遥操作）：
 *   `./build/bin/peripherals_sdk --spacemouse`
 * - 只开六维力传感器：
 *   `./build/bin/peripherals_sdk --ft-sensor`
 * - 显式同时开启两路（与默认等价）：
 *   `./build/bin/peripherals_sdk --spacemouse --ft-sensor`
 *
 * 选择规则：不传 `--spacemouse` / `--ft-sensor` 时默认两路都启动；只要出现
 * 任一选择参数，则仅启动显式选择的外设。
 *
 * `--robot` 可省略；省略时向 SDK 传入空 IP，由 smrcore_sdk 使用默认连接行为。
 */

#include "peripherals/peripherals.hpp"
#include "sdk/robot.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace
{

// ============================================================================
// 设备配置（按现场硬件修改；本示例不提供对应命令行参数）
// ============================================================================

/**
 * SpaceMouse 的 Linux input 设备路径，例如 `/dev/input/event7`。
 * 空字符串表示由外设库自动探测第一个可用 SpaceMouse。
 */
const std::string kSpaceMouseDevicePath = "";

/**
 * SpaceMouse 读取/输出采样率 [Hz]。
 * <= 0 时使用外设库默认值（125 Hz）。
 */
const double kSpaceMouseSampleRateHz = 0.0;

/**
 * 六维力传感器串口设备路径，例如 `/dev/ttyUSB0`。
 * 运行用户需对该节点具备读写权限。
 */
const std::string kFtSensorSerialPort = "/dev/ttyUSB0";

/**
 * 力传感器串口波特率 [bit/s]。
 * 坤维（kunwei_serial）常用 460800；请与传感器手册一致。
 */
const int kFtSensorBaudRate = 460800;

/**
 * 力传感器协议类型字符串。
 * 常见值：`xjc_serial`（鑫精诚）、`kunwei_serial`（坤维）。
 */
const std::string kFtSensorType = "xjc_serial";

/**
 * 力传感器读取/输出采样率 [Hz]。
 * <= 0 时使用外设库默认值（1000 Hz）。
 */
const double kFtSensorSampleRateHz = 1000.0;

/**
 * 力传感器首帧等待的基础超时 [ms]。
 * `WaitForFirstSample()` 实际超时为 max(100, 5 * stale_timeout_ms)。
 */
const int kFtSensorStaleTimeoutMs = 20;

/**
 * SpaceMouse 桥接线程每次轮询后的休眠时间 [ms]。
 * 略大于 125 Hz 周期，避免空转占满 CPU。
 */
const int kSpaceMousePollSleepMs = 8;

/**
 * 力传感器桥接线程每次轮询后的休眠时间 [ms]。
 * 力传感器默认 1 kHz，休眠 1 ms 即可。
 */
const int kFtSensorPollSleepMs = 1;

// ============================================================================
// 进程级状态
// ============================================================================

/** 主循环与各桥接线程的运行标志；收到 SIGINT/SIGTERM 后置 false。 */
std::atomic<bool> g_running{true};

/**
 * 将 POSIX 信号处理为优雅退出请求。
 * @param signal 信号编号（本示例未区分具体信号类型）
 */
void HandleSignal(int)
{
    g_running.store(false, std::memory_order_release);
}

/**
 * 本示例命令行可解析的选项。
 * 外设硬件参数见文件顶部常量，不在此结构中。
 */
struct ExampleOptions
{
    /** 机器人控制器 IP；空字符串表示使用 SDK 默认连接方式。 */
    std::string robot_ip;

    /** 是否启动 SpaceMouse 读取与注入。 */
    bool spacemouse_enabled{true};

    /** 是否启动六维力传感器读取与注入。 */
    bool ft_sensor_enabled{true};

    /** 用户是否请求 `--help` / `-h`。 */
    bool help_requested{false};
};

/**
 * 根据文件顶部常量构造 SpaceMouse 初始化选项。
 * @return 传给 `SpaceMouse::Initialize()` 的选项
 */
smrcore::peripherals::SpaceMouseOptions BuildSpaceMouseOptions()
{
    smrcore::peripherals::SpaceMouseOptions options;
    options.device_path = kSpaceMouseDevicePath;
    options.sample_rate_hz = kSpaceMouseSampleRateHz;
    return options;
}

/**
 * 根据文件顶部常量构造六维力传感器初始化选项。
 * @return 传给 `FtSensor::Initialize()` 的选项
 */
smrcore::peripherals::FtSensorOptions BuildFtSensorOptions()
{
    smrcore::peripherals::FtSensorOptions options;
    options.serial_port = kFtSensorSerialPort;
    options.baud_rate = kFtSensorBaudRate;
    options.sensor_type = kFtSensorType;
    options.sample_rate_hz = kFtSensorSampleRateHz;
    options.stale_timeout_ms = kFtSensorStaleTimeoutMs;
    return options;
}

/**
 * 打印命令行用法到 stderr。
 * @param program 可执行文件路径（argv[0]）
 */
void PrintUsage(const char *program)
{
    std::cerr << "usage: " << program << " [--robot IP] "
              << "[--spacemouse] [--ft-sensor] [--help]\n"
              << "\n"
              << "  --robot IP       robot controller IP (optional)\n"
              << "  --spacemouse     enable SpaceMouse only (with other flags)\n"
              << "  --ft-sensor      enable F/T sensor only (with other flags)\n"
              << "  --help, -h       show this help\n"
              << "\n"
              << "Device paths and serial settings are configured as constants\n"
              << "at the top of peripherals_sdk.cpp.\n";
}

/**
 * 解析命令行，填充 ExampleOptions。
 * 仅处理 `--robot`、`--spacemouse`、`--ft-sensor`、`--help`。
 * @param argc 参数个数
 * @param argv 参数数组
 * @param options 输出：解析结果
 * @return 解析成功为 true；未知参数为 false
 */
bool ParseCli(int argc, char **argv, ExampleOptions &options)
{
    bool explicit_enable = false;

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

/**
 * 将外设库夹爪枚举映射为 rcore SDK 枚举。
 * @param command 外设库 `GripperCommand`
 * @return SDK `GripperCommand`
 */
rcore::GripperCommand ToSdk(smrcore::peripherals::GripperCommand command)
{
    return command == smrcore::peripherals::GripperCommand::Close
               ? rcore::GripperCommand::Close
               : rcore::GripperCommand::Open;
}

/**
 * 将外设库 SpaceMouse 采样转换为 SDK 类型。
 * 字段一一对应，供 `PeripheralApi::UpdateSpaceMouseSample()` 使用。
 * @param sample 外设库归一化采样
 * @return SDK `SpaceMouseSample`
 */
rcore::SpaceMouseSample ToSdk(const smrcore::peripherals::SpaceMouseSample &sample)
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

/**
 * 将外设库六维力/力矩采样转换为 SDK 类型。
 * @param sample 外设库 wrench 采样（传感器协议坐标系）
 * @return SDK `FtSensorSample`
 */
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

/**
 * SpaceMouse 桥接循环：轮询本地 reader，有采样则注入 SDK。
 * 在独立线程中运行，直到 g_running 为 false。
 * @param spacemouse 已 Start 的 SpaceMouse reader
 * @param peripheral `robot.Peripheral()` 返回的注入 API
 * @param sdk_mutex 串行化对 Peripheral API 的调用（SDK 非线程安全）
 */
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
        std::this_thread::sleep_for(
            std::chrono::milliseconds(kSpaceMousePollSleepMs));
    }
}

/**
 * 六维力传感器桥接循环：轮询本地 reader，有采样则注入 SDK。
 * @param sensor 已 Start 且已通过首帧等待的 FtSensor reader
 * @param peripheral `robot.Peripheral()` 返回的注入 API
 * @param sdk_mutex 串行化对 Peripheral API 的调用
 */
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
        std::this_thread::sleep_for(
            std::chrono::milliseconds(kFtSensorPollSleepMs));
    }
}

} // namespace

/**
 * 程序入口：连接机器人 SDK，按需启动外设 reader 与桥接线程。
 * Ctrl+C 触发优雅退出并 join 工作线程。
 */
int main(int argc, char **argv)
{
    ExampleOptions options;
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
    if (!robot.Initialize(options.robot_ip))
    {
        std::cerr << "failed to initialize robot SDK" << std::endl;
        return 1;
    }

    smrcore::peripherals::SpaceMouse spacemouse;
    if (options.spacemouse_enabled)
    {
        const auto spacemouse_options = BuildSpaceMouseOptions();
        if (!spacemouse.Initialize(spacemouse_options) || !spacemouse.Start())
        {
            std::cerr << "failed to start SpaceMouse reader" << std::endl;
            return 1;
        }
    }

    smrcore::peripherals::FtSensor sensor;
    if (options.ft_sensor_enabled)
    {
        const auto ft_options = BuildFtSensorOptions();
        if (!sensor.Initialize(ft_options) || !sensor.Start())
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
        ft_sensor_thread = std::thread(RunFtSensorBridge,
                                       std::ref(sensor),
                                       std::ref(peripheral),
                                       std::ref(sdk_mutex));
    }

    if (spacemouse_thread.joinable())
    {
        spacemouse_thread.join();
    }
    if (ft_sensor_thread.joinable())
    {
        ft_sensor_thread.join();
    }
    return 0;
}
