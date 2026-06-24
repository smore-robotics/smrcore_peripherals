/**
 * @file latest_slot.hpp
 * @brief consume-on-read 最新值槽
 */

#pragma once

#include <mutex>
#include <optional>

namespace smrcore::peripherals::internal
{

/**
 * @brief 线程安全的单槽缓冲：写入覆盖、读取取走并清空
 */
template <typename T>
class LatestSlot
{
public:
    void Set(const T &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ = value;
    }

    std::optional<T> Take()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!value_)
        {
            return std::nullopt;
        }
        T copy = *value_;
        value_.reset();
        return copy;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        value_.reset();
    }

private:
    std::mutex mutex_;
    std::optional<T> value_;
};

} // namespace smrcore::peripherals::internal
