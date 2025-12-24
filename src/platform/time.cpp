#include "eunet/platform/time.h"

namespace platform::time
{
    MonoPoint monotonic_now() { return MonoClock::now(); }

    WallPoint wall_now() { return WallClock::now(); }

    Duration elapsed(MonoPoint from, MonoPoint to) { return std::chrono::duration_cast<Duration>(to - from); }
    Duration since(MonoPoint from) { return elapsed(from, monotonic_now()); }

    UnixTime to_unix(WallPoint tp) { return WallClock::to_time_t(tp); }
    WallPoint from_unix(UnixTime t) { return WallClock::from_time_t(t); }

    MonoPoint deadline_after(Duration d) { return monotonic_now() + d; }
    bool expired(MonoPoint deadline) { return monotonic_now() >= deadline; }
}