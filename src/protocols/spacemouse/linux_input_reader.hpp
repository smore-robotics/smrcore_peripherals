/**
 * @file linux_input_reader.hpp
 * @brief SpaceMouse / 3Dconnexion Linux input 子系统读取（协议层）
 *
 * 通过 `/dev/input/event*` 读取 `EV_ABS` / `EV_KEY` 原始事件，
 * 聚合为 6 轴 + 按键位掩码的 `RawInputFrame`。设备发现依赖
 * udev 名称与 USB vendor 白名单。
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace smrcore::peripherals::protocols::spacemouse
{

/**
 * @struct DeviceInfo
 * @brief 扫描到的 SpaceMouse 类输入设备描述
 */
struct DeviceInfo
{
    std::string event_path; ///< 如 `/dev/input/event5`
    std::string name;       ///< `EVIOCGNAME` 返回的设备名
    uint16_t vendor{0};     ///< USB vendor id
    uint16_t product{0};    ///< USB product id
};

/**
 * @struct RawInputFrame
 * @brief 一帧聚合后的原始输入（未归一化）
 *
 * `axes` 顺序与 Linux ABS 轴映射一致，由 `LinuxInputReader` 内部维护；
 * `buttons` 为按键位掩码。
 */
struct RawInputFrame
{
    std::array<int32_t, 6> axes{{0, 0, 0, 0, 0, 0}};
    uint32_t buttons{0};
    double timestamp_sec{0.0};
};

/**
 * @class LinuxInputReader
 * @brief 阻塞/限时读取单路 evdev 设备
 *
 * - `ScanDevices()`：遍历 `/dev/input/event*`，过滤 SpaceMouse 名称或 3Dconnexion vendor
 * - `Open()` / `ReadFrame()`：打开设备并同步读取一帧；绝对轴与按键在类内累积
 *
 * 线程安全：非线程安全。
 */
class LinuxInputReader
{
public:
    LinuxInputReader() = default;
    ~LinuxInputReader();

    LinuxInputReader(const LinuxInputReader &) = delete;
    LinuxInputReader &operator=(const LinuxInputReader &) = delete;

    /**
     * @brief 枚举本机可用的 SpaceMouse 类设备
     * @return 可能为空；非 Linux 平台恒返回空列表
     */
    static std::vector<DeviceInfo> ScanDevices();

    /**
     * @brief 打开指定 event 节点
     * @param event_path 如 `/dev/input/event5`
     */
    bool Open(const std::string &event_path);

    void Close();
    bool IsOpen() const;

    /**
     * @brief 读取并聚合为一帧
     * @param frame 输出帧
     * @param timeout_ms `poll` 等待毫秒数
     * @param disconnected 可选；设备断开时置 true
     * @return 是否得到至少一个有效轴/按键更新
     */
    bool ReadFrame(RawInputFrame &frame, int timeout_ms,
                   bool *disconnected = nullptr);

private:
    int fd_{-1};
    uint32_t buttons_{0};
    RawInputFrame pending_;
    std::array<int32_t, 6> absolute_axes_{{0, 0, 0, 0, 0, 0}};
    bool frame_has_absolute_axis_{false};
};

} // namespace smrcore::peripherals::protocols::spacemouse
