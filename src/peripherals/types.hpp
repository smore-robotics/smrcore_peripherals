/**
 * @file types.hpp
 * @brief 外设库公共数据类型
 *
 * 类型语义与 rcore SDK `data.hpp` 中的外设采样类型保持一致，便于桥接
 * `robot.Peripheral().UpdateSpaceMouseSample()` / `UpdateFtSensorSample()`。
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include <array>
#include <cstdint>
#include <ostream>
#include <string>

namespace smrcore::peripherals
{

/**
 * @brief SpaceMouse 夹爪目标状态（锁存值，非边沿事件）
 *
 * 读取器启动时默认为 Open；在配置的闭合按键上升沿切换为 Close，
 * 之后持续发布最新目标，直到下一次有效边沿改变。
 */
enum class GripperCommand : uint8_t
{
    Open = 0,  ///< 张开目标；rcore 映射为配置的张开位置
    Close = 1, ///< 闭合目标；rcore 映射为配置的闭合位置
};

/** @brief 外设初始化公共选项 */
struct PeripheralOptions
{
    /**
     * 请求的输出/采样频率 [Hz]。<= 0 时使用设备默认值：
     * SpaceMouse 125 Hz，力传感器 1000 Hz。
     */
    double sample_rate_hz{0.0};
};

/**
 * @brief 力传感器初始化选项
 *
 * 由 `FtSensor::Initialize()` 消费。运行中不支持原地修改，需 `Stop()` →
 * `Shutdown()` 后以新参数重新 `Initialize()`。
 */
struct FtSensorOptions : public PeripheralOptions
{
    /** 力传感器 POSIX 串口，例如 `/dev/ttyUSB0`。 */
    std::string serial_port;

    /** 力传感器串口波特率 [bit/s]，坤维默认 460800。 */
    int baud_rate{460800};

    /** 力传感器串口单次 read 缓冲区大小 [byte]，默认 512。 */
    std::size_t read_buffer_size{512};

    /** 力传感器协议类型，如 `kunwei_serial`、`xjc_serial`。 */
    std::string sensor_type{"xjc_serial"};

    /** 首帧等待基础超时时间 [ms]，实际超时为 `max(100, 5 * stale_timeout_ms)`。 */
    int stale_timeout_ms{20};
};

/**
 * @brief SpaceMouse 初始化选项
 *
 * 由 `SpaceMouse::Initialize()` 消费。运行中不支持原地修改，需 `Stop()` →
 * `Shutdown()` 后以新参数重新 `Initialize()`。
 */
struct SpaceMouseOptions : public PeripheralOptions
{

    /**
     * SpaceMouse 的 Linux input 设备路径，例如 `/dev/input/event7`。
     * 空字符串表示自动探测。生命周期：在 Initialize() 时拷贝。
     */
    std::string device_path;

    /**
     * SpaceMouse 输出轴到原始轴的映射。`axis_map[i]` 指定归一化输出 i
     * （x,y,z,roll,pitch,yaw）对应的原始轴索引；无效索引输出为零。
     */
    std::array<int, 6> axis_map{{0, 1, 2, 3, 4, 5}};

    /**
     * 各输出轴符号。>= 0 保持原符号，< 0 取反。默认与 rcore 遗留映射一致：
     * Y/Z 与 pitch/yaw 取反。
     */
    std::array<int, 6> axis_sign{{1, -1, -1, 1, -1, -1}};

    /**
     * 各输出轴归一化满量程对应的原始计数值，绝对值 < 1e-9 时按 1.0 处理。
     */
    std::array<double, 6> axis_scale{{350.0, 350.0, 350.0,
                                      350.0, 350.0, 350.0}};

    /** 归一化死区 [0, 1]，绝对值低于此阈值的轴输出为零。 */
    double deadzone{0.01};
};

/**
 * @brief 低频设备/状态信息（不进高频采样流）
 *
 * 供 UI、日志与诊断使用。字符串为 OS 或协议上报的 UTF-8 快照。
 */
struct PeripheralInfo
{
    /** 当前设备路径或串口，空表示未选定 */
    std::string path;

    /** 设备型号/名称，未知时为空 */
    std::string model;

    /** 序列号，未知时为空 */
    std::string serial_number;

    /** 固件版本，未知时为空 */
    std::string firmware_version;

    /** Initialize() 选定的有效采样率 [Hz] */
    double sample_rate_hz{0.0};

    /** 本地读取器是否认为设备已连接（管理面状态） */
    bool connected{false};

    /** 自上次 Initialize() 或解析器重置以来的帧错误计数（力传感器更新） */
    uint32_t frame_error_count{0};
};

/**
 * @brief 归一化 SpaceMouse 采样
 *
 * XYZRPY 为输入通道名，非机器人坐标系语义；rcore 按遥操作配置解释。
 * 与 rcore SDK `SpaceMouseSample` 字段一一对应。
 */
struct SpaceMouseSample
{
    /** 归一化 X 平移通道，范围 [-1, 1] */
    double x{0.0};
    /** 归一化 Y 平移通道，范围 [-1, 1] */
    double y{0.0};
    /** 归一化 Z 平移通道，范围 [-1, 1] */
    double z{0.0};
    /** 归一化 roll 通道，范围 [-1, 1] */
    double roll{0.0};
    /** 归一化 pitch 通道，范围 [-1, 1] */
    double pitch{0.0};
    /** 归一化 yaw 通道，范围 [-1, 1] */
    double yaw{0.0};
    /** 锁存夹爪目标状态，随每条采样一并携带 */
    GripperCommand gripper_command{GripperCommand::Open};
    /**
     * 生产者单调时钟时间戳 [s]，仅元数据；
     * rcore 控制器以本地 DDS 到达时间判断新鲜度。
     */
    double timestamp_sec{0.0};
};

/** @brief 归一化前的 SpaceMouse 原始采样 */
struct SpaceMouseRawSample
{
    /**
     * 设备通道顺序的原始轴计数值。Linux EV_REL 设备由读取器累加为六轴状态。
     */
    std::array<int32_t, 6> axes{{0, 0, 0, 0, 0, 0}};

    /** 原始按键位掩码，默认 bit0=闭合、bit1=张开 */
    uint32_t buttons{0};

    /** 生产者单调时钟时间戳 [s] */
    double timestamp_sec{0.0};
};

/**
 * @brief 力传感器协议坐标系下的原始六维力/力矩
 *
 * 与 rcore SDK `FtSensorSample` 字段一一对应。坤维 V1 解析器将协议
 * kg/kg·m 浮点转为 N/N·m，不施加法兰/工具变换或标定补偿。
 */
struct FtSensorSample
{
    /** 力 X [N]，传感器协议坐标系 */
    double fx{0.0};
    /** 力 Y [N]，传感器协议坐标系 */
    double fy{0.0};
    /** 力 Z [N]，传感器协议坐标系 */
    double fz{0.0};
    /** 力矩 X [N·m]，传感器协议坐标系 */
    double tx{0.0};
    /** 力矩 Y [N·m]，传感器协议坐标系 */
    double ty{0.0};
    /** 力矩 Z [N·m]，传感器协议坐标系 */
    double tz{0.0};
    /**
     * 生产者单调时钟时间戳 [s]，仅元数据；
     * rcore 控制器以本地 DDS 到达时间判断新鲜度。
     */
    double timestamp_sec{0.0};
};

inline std::ostream &operator<<(std::ostream &os, const SpaceMouseSample &sample)
{
    return os << sample.x << ' ' << sample.y << ' ' << sample.z << ' ' << sample.roll << ' '
              << sample.pitch << ' ' << sample.yaw << ' '
              << static_cast<uint8_t>(sample.gripper_command) << ' ' << sample.timestamp_sec;
}

inline std::ostream &operator<<(std::ostream &os, const SpaceMouseRawSample &sample)
{
    os << "buttons=" << sample.buttons << " axes=";
    for (std::size_t i = 0; i < sample.axes.size(); ++i)
    {
        if (i > 0)
        {
            os << ' ';
        }
        os << sample.axes[i];
    }
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const FtSensorSample &sample)
{
    return os << sample.fx << ' ' << sample.fy << ' ' << sample.fz << ' ' << sample.tx << ' '
              << sample.ty << ' ' << sample.tz;
}

} // namespace smrcore::peripherals
