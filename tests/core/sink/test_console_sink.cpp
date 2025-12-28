#include <cassert>
#include <iostream>
#include <sstream>

#include "eunet/core/event.hpp"
#include "eunet/core/lifecycle_fsm.hpp"
#include "eunet/core/event_snapshot.hpp"
#include "eunet/core/sink/console_sink.hpp"

using namespace core;
using namespace sink;

static EventSnapshot make_snapshot(
    int fd,
    LifeState state,
    bool error = false)
{
    Event e =
        error
            ? Event::failure(
                  EventType::HTTP_SENT,
                  util::Error::internal("send failed"),
                  {fd})
            : Event::info(
                  EventType::HTTP_SENT,
                  "send ok",
                  {fd});

    static LifecycleFSM dummy_fsm(fd);

    return EventSnapshot{
        .event = &e,
        .fd = fd,
        .state = state,
        .ts = e.ts,
        .has_error = error,
        .error = e.error,
    };
}

int main()
{
    ConsoleSink sink;

    std::ostringstream buffer;
    auto *old_buf = std::cout.rdbuf(buffer.rdbuf());

    {
        auto snap = make_snapshot(5, LifeState::Sending, false);
        sink.on_event(snap);
    }

    {
        auto snap = make_snapshot(6, LifeState::Error, true);
        sink.on_event(snap);
    }

    std::cout.rdbuf(old_buf);

    std::string output = buffer.str();

    // 基本存在性
    assert(!output.empty());

    // fd 是否被打印
    assert(output.find("fd=5") != std::string::npos);
    assert(output.find("fd=6") != std::string::npos);

    // 状态字符串是否出现
    assert(output.find(to_string(LifeState::Sending)) != std::string::npos);
    assert(output.find(to_string(LifeState::Error)) != std::string::npos);

    std::cout.rdbuf(old_buf);
    std::cout << buffer.str();
    std::cout << "[OK] ConsoleSink output test passed\n";
    return 0;
}
