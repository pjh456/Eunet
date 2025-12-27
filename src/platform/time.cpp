#include "eunet/platform/time.hpp"
#include <thread>
#include <iomanip>
#include <sstream>

namespace platform::time
{
    MonoPoint monotonic_now() { return MonoClock::now(); }
    std::string to_string(MonoPoint tp)
    {
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
        std::ostringstream oss;
        oss << ns << "ns";
        return oss.str();
    }

    WallPoint wall_now() { return WallClock::now(); }
    std::string to_string(WallPoint tp)
    {
        auto t = WallClock::to_time_t(tp);
        std::tm tm{};
        localtime_r(&t, &tm); // 线程安全
        std::ostringstream oss;
        oss << std::put_time(&tm, "%F %T"); // 2025-12-27 10:30:15
        return oss.str();
    }

    Duration elapsed(MonoPoint from, MonoPoint to) { return std::chrono::duration_cast<Duration>(to - from); }
    Duration since(MonoPoint from) { return elapsed(from, monotonic_now()); }

    UnixTime to_unix(WallPoint tp) { return WallClock::to_time_t(tp); }
    WallPoint from_unix(UnixTime t) { return WallClock::from_time_t(t); }

    MonoPoint deadline_after(Duration d) { return monotonic_now() + d; }
    bool expired(MonoPoint deadline) { return monotonic_now() >= deadline; }

    void sleep_for(Duration d)
    {
        if (d.count() <= 0)
            return;

        std::this_thread::sleep_for(d);
    }
    void sleep_until(MonoPoint tp)
    {
        std::this_thread::sleep_until(tp);
    }
}