#include <cassert>
#include <iostream>

#include "eunet/core/event.hpp"
#include "eunet/core/lifecycle_fsm.hpp"
#include "eunet/core/event_snapshot.hpp"
#include "eunet/core/sink/metrics_sink.hpp"

using namespace core;
using namespace core::sink;

static EventSnapshot make_snapshot(
    int fd,
    LifeState state,
    bool error = false)
{
    Event e =
        error
            ? Event::failure(
                  EventType::TCP_CONNECT_START,
                  util::Error::internal("connect failed"),
                  {fd})
            : Event::info(
                  EventType::TCP_CONNECT_START,
                  "connect ok",
                  {fd});

    static LifecycleFSM dummy_fsm(fd);

    return EventSnapshot{
        .event = e,
        .fd = fd,
        .state = state,
        .ts = e.ts,
        .error = e.error,
    };
}

int main()
{
    MetricsSink sink;

    {
        auto snap = make_snapshot(3, LifeState::Connecting, false);
        sink.on_event(snap);
    }

    {
        auto snap = make_snapshot(3, LifeState::Established, false);
        sink.on_event(snap);
    }

    {
        auto snap = make_snapshot(4, LifeState::Connecting, true);
        sink.on_event(snap);
    }

    const auto &m = sink.snapshot();

    assert(m.total_events == 3);
    assert(m.errors == 1);

    std::cout << "[OK] MetricsSink basic counting test passed\n";
    return 0;
}
