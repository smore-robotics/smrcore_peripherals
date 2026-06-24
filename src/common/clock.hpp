/**
 * @file clock.hpp
 * @brief 单调时钟工具
 */

#pragma once

#include <chrono>

namespace smrcore::peripherals::internal
{

/** @return 自任意起点的单调时钟秒数 */
inline double NowSeconds()
{
    using clock = std::chrono::steady_clock;
    const auto now = clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

} // namespace smrcore::peripherals::internal
