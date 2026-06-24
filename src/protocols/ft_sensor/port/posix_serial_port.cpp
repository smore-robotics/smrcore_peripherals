/**
 * @file posix_serial_port.cpp
 * @brief POSIX 串口底层读写实现
 */

#include "protocols/ft_sensor/port/posix_serial_port.hpp"

#include <cerrno>

#ifndef _WIN32
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace smrcore::peripherals::protocols::ft_sensor::port
{

namespace
{

#ifndef _WIN32
bool BaudToSpeed(int baud_rate, speed_t &speed)
{
    switch (baud_rate)
    {
    case 9600:
        speed = B9600;
        return true;
    case 19200:
        speed = B19200;
        return true;
    case 38400:
        speed = B38400;
        return true;
    case 57600:
        speed = B57600;
        return true;
    case 115200:
        speed = B115200;
        return true;
#ifdef B230400
    case 230400:
        speed = B230400;
        return true;
#endif
#ifdef B460800
    case 460800:
        speed = B460800;
        return true;
#endif
#ifdef B921600
    case 921600:
        speed = B921600;
        return true;
#endif
    default:
        return false;
    }
}
#endif

void SetDisconnected(bool *disconnected, bool value)
{
    if (disconnected)
    {
        *disconnected = value;
    }
}

} // namespace

PosixSerialPort::~PosixSerialPort() { Close(); }

bool PosixSerialPort::Open(const std::string &path, int baud_rate)
{
    Close();
#ifndef _WIN32
    speed_t speed{};
    if (!BaudToSpeed(baud_rate, speed))
    {
        return false;
    }

    fd_ = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0)
    {
        return false;
    }

    termios tio{};
    if (::tcgetattr(fd_, &tio) != 0)
    {
        Close();
        return false;
    }

    ::cfmakeraw(&tio);
    ::cfsetispeed(&tio, speed);
    ::cfsetospeed(&tio, speed);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;
    tio.c_cflag &= ~PARENB;
    tio.c_cflag &= ~CSTOPB;
#ifdef CRTSCTS
    tio.c_cflag &= ~CRTSCTS;
#endif
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    if (::tcsetattr(fd_, TCSANOW, &tio) != 0)
    {
        Close();
        return false;
    }
    ::tcflush(fd_, TCIOFLUSH);
    return true;
#else
    (void)path;
    (void)baud_rate;
    return false;
#endif
}

void PosixSerialPort::Close()
{
#ifndef _WIN32
    if (fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

bool PosixSerialPort::IsOpen() const { return fd_ >= 0; }

bool PosixSerialPort::WriteAll(const uint8_t *data, std::size_t size)
{
#ifndef _WIN32
    if (fd_ < 0 || (data == nullptr && size > 0))
    {
        return false;
    }
    std::size_t offset = 0;
    while (offset < size)
    {
        const ssize_t written =
            ::write(fd_, data + offset, static_cast<size_t>(size - offset));
        if (written > 0)
        {
            offset += static_cast<std::size_t>(written);
            continue;
        }
        if (written < 0 && errno == EINTR)
        {
            continue;
        }
        if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            pollfd pfd{};
            pfd.fd = fd_;
            pfd.events = POLLOUT;
            const int ready = ::poll(&pfd, 1, 100);
            if (ready > 0 && (pfd.revents & POLLOUT) != 0)
            {
                continue;
            }
        }
        return false;
    }
    return true;
#else
    (void)data;
    (void)size;
    return false;
#endif
}

int PosixSerialPort::ReadSome(uint8_t *buffer, std::size_t size,
                              int timeout_ms, bool *disconnected)
{
    SetDisconnected(disconnected, false);
#ifndef _WIN32
    if (fd_ < 0 || buffer == nullptr || size == 0)
    {
        SetDisconnected(disconnected, fd_ < 0);
        return -1;
    }

    while (true)
    {
        pollfd pfd{};
        pfd.fd = fd_;
        pfd.events = POLLIN;
        const int ready = ::poll(&pfd, 1, timeout_ms);
        if (ready < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            SetDisconnected(disconnected, true);
            return -1;
        }
        if (ready == 0)
        {
            return 0;
        }
        if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
        {
            SetDisconnected(disconnected, true);
            return -1;
        }
        if ((pfd.revents & POLLIN) == 0)
        {
            return 0;
        }

        const ssize_t n = ::read(fd_, buffer, size);
        if (n > 0)
        {
            return static_cast<int>(n);
        }
        if (n == 0)
        {
            return 0;
        }
        if (errno == EINTR)
        {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0;
        }
        SetDisconnected(disconnected, true);
        return -1;
    }
#else
    (void)buffer;
    (void)size;
    (void)timeout_ms;
    SetDisconnected(disconnected, true);
    return -1;
#endif
}

void PosixSerialPort::FlushInput()
{
#ifndef _WIN32
    if (fd_ >= 0)
    {
        ::tcflush(fd_, TCIFLUSH);
    }
#endif
}

} // namespace smrcore::peripherals::protocols::ft_sensor::port
