/*
 * ============================================================================
 *  File Name   : time.hpp
 *  Module      : platform/time
 *
 *  Description :
 *      时间与时钟的封装。区分单调时钟 (Monotonic Clock，用于计时)
 *      和墙上时钟 (Wall Clock，用于展示)。提供时间点计算和格式化功能。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_PLATFORM_TIME
#define INCLUDE_EUNET_PLATFORM_TIME

#include <chrono>

namespace platform::time
{
    // 单调时间（用于计时）
    using MonoClock = std::chrono::steady_clock;
    using MonoPoint = MonoClock::time_point;
    MonoPoint monotonic_now();

    // 现实时间（系统时间）
    using WallClock = std::chrono::system_clock;
    using WallPoint = WallClock::time_point;
    WallPoint wall_now();

    // 时间间隔
    using Duration = std::chrono::milliseconds;
    Duration elapsed(MonoPoint start, MonoPoint end);
    Duration since(MonoPoint start);

    // Unix 时间
    using UnixTime = std::time_t;
    UnixTime to_unix(WallPoint tp);
    WallPoint from_unix(UnixTime t);

    MonoPoint deadline_after(Duration d);
    bool expired(MonoPoint deadline);

    void sleep_for(Duration d);
    void sleep_until(MonoPoint tp);

}

std::string to_string(platform::time::MonoPoint tp);
std::string to_string(platform::time::WallPoint tp);

#endif // INCLUDE_EUNET_PLATFORM_TIME