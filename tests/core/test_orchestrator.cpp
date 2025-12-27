#include <cassert>
#include <iostream>

#include "eunet/core/orchestrator.hpp"
#include "eunet/core/lifecycle_fsm.hpp"
#include "eunet/core/event.hpp"

using namespace core;

// ---------------- FakeSink ----------------

struct FakeSink : sink::IEventSink
{
    struct Record
    {
        EventType type;
        int fd;
        LifeState state;
    };

    std::vector<Record> records;

    void on_event(
        const Event &e,
        const LifecycleFSM &fsm) override
    {
        records.push_back(
            {e.type,
             e.fd,
             fsm.current_state()});
    }
};

// ---------------- Test ----------------

int main()
{
    Orchestrator orch;
    FakeSink sink;

    orch.attach(&sink);

    const int fd = 3;

    orch.emit(Event::info(EventType::DNS_START, "dns start", fd));
    orch.emit(Event::info(EventType::DNS_DONE, "dns done", fd));
    orch.emit(Event::info(EventType::TCP_CONNECT, "connect", fd));
    orch.emit(Event::info(EventType::TCP_ESTABLISHED, "established", fd));
    orch.emit(Event::info(EventType::REQUEST_SENT, "send", fd));
    orch.emit(Event::info(EventType::REQUEST_RECEIVED, "recv", fd));

    // ---------- Sink call count ----------
    assert(sink.records.size() == 6);

    // ---------- FSM state progression ----------
    assert(sink.records[0].state == LifeState::Resolving);
    assert(sink.records[1].state == LifeState::Resolving || sink.records[1].state == LifeState::Connecting);

    assert(sink.records[2].state == LifeState::Connecting);
    assert(sink.records[3].state == LifeState::Established);
    assert(sink.records[4].state == LifeState::Sending);
    assert(sink.records[5].state == LifeState::Receiving || sink.records[5].state == LifeState::Finished);

    // ---------- Timeline consistency ----------
    const auto &timeline = orch.get_timeline();
    assert(timeline.size() == 6);

    auto by_fd = timeline.query_by_fd(fd);
    assert(by_fd.size() == 6);

    // ---------- FSM final state ----------
    const auto *fsm = orch.get_fsm(fd);
    assert(fsm != nullptr);
    assert(fsm->current_state() == LifeState::Finished || fsm->current_state() == LifeState::Receiving);

    std::cout << "[OK] Orchestrator basic flow test passed.\n";
    return 0;
}
