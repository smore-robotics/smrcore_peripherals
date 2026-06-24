/**
 * @file linux_input_reader.cpp
 * @brief SpaceMouse Linux input 读取实现（协议层）
 */

#include "linux_input_reader.hpp"

#include "common/clock.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <filesystem>
#include <string>

#ifndef _WIN32
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace smrcore::peripherals::protocols::spacemouse
{

namespace
{

bool IsSpaceMouseName(const std::string &name)
{
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });
    return lower.find("spacemouse") != std::string::npos ||
           lower.find("spacenavigator") != std::string::npos ||
           lower.find("3dconnexion") != std::string::npos ||
           lower.find("space navigator") != std::string::npos;
}

bool Is3Dconnexion(uint16_t vendor)
{
    return vendor == 0x046d || vendor == 0x256f;
}

#ifndef _WIN32
bool TestBit(const std::vector<unsigned long> &bits, int bit)
{
    constexpr int kBitsPerWord = static_cast<int>(sizeof(unsigned long) * 8);
    const int word = bit / kBitsPerWord;
    const int offset = bit % kBitsPerWord;
    return word >= 0 && word < static_cast<int>(bits.size()) &&
           ((bits[static_cast<std::size_t>(word)] >> offset) & 1UL) != 0;
}

bool HasAxisSet(int fd, int event_type, const std::array<int, 6> &codes,
                int max_code)
{
    const std::size_t word_count =
        static_cast<std::size_t>(max_code / (sizeof(unsigned long) * 8) + 1);
    std::vector<unsigned long> bits(word_count, 0);
    if (::ioctl(fd,
                EVIOCGBIT(event_type, static_cast<int>(bits.size() *
                                                       sizeof(unsigned long))),
                bits.data()) < 0)
    {
        return false;
    }
    return std::all_of(codes.begin(), codes.end(),
                       [&](int code) { return TestBit(bits, code); });
}

bool HasSpaceMouseAxes(int fd)
{
    constexpr std::array<int, 6> kRelAxes = {REL_X,  REL_Y,  REL_Z,
                                             REL_RX, REL_RY, REL_RZ};
    constexpr std::array<int, 6> kAbsAxes = {ABS_X,  ABS_Y,  ABS_Z,
                                             ABS_RX, ABS_RY, ABS_RZ};
    return HasAxisSet(fd, EV_REL, kRelAxes, REL_MAX) ||
           HasAxisSet(fd, EV_ABS, kAbsAxes, ABS_MAX);
}
#endif

} // namespace

LinuxInputReader::~LinuxInputReader() { Close(); }

std::vector<DeviceInfo> LinuxInputReader::ScanDevices()
{
    std::vector<DeviceInfo> result;
#ifndef _WIN32
    const std::filesystem::path input_dir("/dev/input");
    if (!std::filesystem::exists(input_dir))
    {
        return result;
    }

    for (const auto &entry : std::filesystem::directory_iterator(input_dir))
    {
        const auto path = entry.path();
        const std::string filename = path.filename().string();
        if (filename.rfind("event", 0) != 0)
        {
            continue;
        }

        const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0)
        {
            continue;
        }

        char name[256] = {};
        struct input_id id
        {
        };
        (void)::ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        (void)::ioctl(fd, EVIOCGID, &id);
        const bool has_spacemouse_axes = HasSpaceMouseAxes(fd);
        ::close(fd);

        const std::string device_name(name);
        if (!IsSpaceMouseName(device_name) && !Is3Dconnexion(id.vendor))
        {
            continue;
        }
        if (!has_spacemouse_axes)
        {
            continue;
        }

        DeviceInfo info;
        info.event_path = path.string();
        info.name = device_name;
        info.vendor = id.vendor;
        info.product = id.product;
        result.push_back(info);
    }
    std::sort(result.begin(), result.end(),
              [](const DeviceInfo &lhs,
                 const DeviceInfo &rhs)
              { return lhs.event_path < rhs.event_path; });
#endif
    return result;
}

bool LinuxInputReader::Open(const std::string &event_path)
{
    Close();
#ifndef _WIN32
    fd_ = ::open(event_path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    return fd_ >= 0;
#else
    (void)event_path;
    return false;
#endif
}

void LinuxInputReader::Close()
{
#ifndef _WIN32
    if (fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

bool LinuxInputReader::IsOpen() const { return fd_ >= 0; }

bool LinuxInputReader::ReadFrame(RawInputFrame &frame,
                                           int timeout_ms,
                                           bool *disconnected)
{
#ifndef _WIN32
    if (disconnected)
    {
        *disconnected = false;
    }
    if (fd_ < 0)
    {
        if (disconnected)
        {
            *disconnected = true;
        }
        return false;
    }

    pending_.buttons = buttons_;

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
            if (disconnected)
            {
                *disconnected = true;
            }
            return false;
        }
        if (ready == 0)
        {
            return false;
        }
        if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
        {
            if (disconnected)
            {
                *disconnected = true;
            }
            return false;
        }

        input_event ev{};
        const ssize_t n = ::read(fd_, &ev, sizeof(ev));
        if (n != static_cast<ssize_t>(sizeof(ev)))
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return false;
            }
            if (disconnected)
            {
                *disconnected = true;
            }
            return false;
        }

        if (ev.type == EV_SYN && ev.code == SYN_REPORT)
        {
            pending_.timestamp_sec = smrcore::peripherals::internal::NowSeconds();
            if (frame_has_absolute_axis_)
            {
                pending_.axes = absolute_axes_;
            }
            frame = pending_;
            pending_.axes = {{0, 0, 0, 0, 0, 0}};
            pending_.buttons = buttons_;
            frame_has_absolute_axis_ = false;
            return true;
        }
        if (ev.type == EV_KEY)
        {
            const int bit = ev.code - BTN_0;
            if (bit >= 0 && bit < 32)
            {
                if (ev.value)
                {
                    buttons_ |= (1u << bit);
                }
                else
                {
                    buttons_ &= ~(1u << bit);
                }
                pending_.buttons = buttons_;
            }
            continue;
        }
        if (ev.type != EV_REL && ev.type != EV_ABS)
        {
            continue;
        }

        std::array<int32_t, 6> *target_axes = &pending_.axes;
        if (ev.type == EV_ABS)
        {
            target_axes = &absolute_axes_;
            frame_has_absolute_axis_ = true;
        }

        switch (ev.code)
        {
        case REL_X:
            (*target_axes)[0] = ev.value;
            break;
        case REL_Y:
            (*target_axes)[1] = ev.value;
            break;
        case REL_Z:
            (*target_axes)[2] = ev.value;
            break;
        case REL_RX:
            (*target_axes)[3] = ev.value;
            break;
        case REL_RY:
            (*target_axes)[4] = ev.value;
            break;
        case REL_RZ:
            (*target_axes)[5] = ev.value;
            break;
        default:
            break;
        }
    }
#else
    (void)frame;
    (void)timeout_ms;
    if (disconnected)
    {
        *disconnected = true;
    }
#endif
    return false;
}

} // namespace smrcore::peripherals::protocols::spacemouse
