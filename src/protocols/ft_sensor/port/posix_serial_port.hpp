/**
 * @file posix_serial_port.hpp
 * @brief POSIX 串口底层读写封装（力传感器传输层）
 *
 * 提供非阻塞 open/read/write/close，供 `SerialReader` 及品牌协议层复用。
 * 仅 Linux/POSIX 可用；Windows 下接口存在但操作恒失败。
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace smrcore::peripherals::protocols::ft_sensor::port
{

/**
 * @class PosixSerialPort
 * @brief 基于 termios 的 8N1 原始模式串口
 *
 * - `Open()`：配置波特率、非阻塞、清空缓冲
 * - `WriteAll()`：循环写入直至完成或超时
 * - `ReadSome()`：`poll` 等待后读取至多 `size` 字节；`timeout_ms==0` 不阻塞
 *
 * 线程安全：非线程安全，同一实例应由单线程或外部同步使用。
 */
class PosixSerialPort
{
public:
    PosixSerialPort() = default;
    ~PosixSerialPort();

    PosixSerialPort(const PosixSerialPort &) = delete;
    PosixSerialPort &operator=(const PosixSerialPort &) = delete;

    /**
     * @brief 打开串口设备
     * @param path 设备路径，如 `/dev/ttyUSB0`
     * @param baud_rate 波特率；不支持的非标准值返回 false
     * @return 是否成功打开并完成 termios 配置
     */
    bool Open(const std::string &path, int baud_rate);

    /** @brief 关闭已打开的文件描述符（可重复调用） */
    void Close();

    /** @return 文件描述符有效且未关闭 */
    bool IsOpen() const;

    /**
     * @brief 写入全部字节
     * @param data 待写缓冲区；`size==0` 时视为成功
     * @param size 字节数
     * @return 是否完整写出
     */
    bool WriteAll(const uint8_t *data, std::size_t size);

    /**
     * @brief 非阻塞/限时读取
     * @param buffer 输出缓冲区
     * @param size 缓冲区容量
     * @param timeout_ms 首次读等待毫秒数；后续轮次由调用方传 0
     * @param disconnected 可选；设备断开或 poll/read 错误时置 true
     * @return 实际读取字节数；0 表示超时无数据；-1 表示错误
     */
    int ReadSome(uint8_t *buffer, std::size_t size, int timeout_ms,
                 bool *disconnected = nullptr);

    /** @brief 丢弃接收缓冲区中尚未读取的字节 */
    void FlushInput();

private:
    int fd_{-1};
};

} // namespace smrcore::peripherals::protocols::ft_sensor::port
