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
    using Duration = std::chrono::nanoseconds;
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