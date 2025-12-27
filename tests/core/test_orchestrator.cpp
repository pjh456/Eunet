#include <cassert>
#include <iostream>

#include "eunet/core/orchestrator.hpp"
#include "eunet/core/lifecycle_fsm.hpp"
#include "eunet/core/event.hpp"

using namespace core;

// ---------------- FakeSink ----------------

struct FakeSink : sink::IEventSink
{

    std::vector<EventSnapshot> records;

    void on_event(const EventSnapshot &snap) override
    {
        records.push_back(snap);
    }
};

// ---------------- Test ----------------

int main()
{
    Orchestrator orch;
    FakeSink sink;

    orch.attach(&sink);

    const int fd = 3;

    auto dns_start = orch.emit(Event::info(EventType::DNS_START, "dns start", fd));
    assert(dns_start.is_ok());
    auto dns_done = orch.emit(Event::info(EventType::DNS_DONE, "dns done", fd));
    assert(dns_done.is_ok());
    auto connect = orch.emit(Event::info(EventType::TCP_CONNECT, "connect", fd));
    assert(connect.is_ok());
    auto established = orch.emit(Event::info(EventType::TCP_ESTABLISHED, "established", fd));
    assert(established.is_ok());
    auto send = orch.emit(Event::info(EventType::REQUEST_SENT, "send", fd));
    assert(send.is_ok());
    auto recv = orch.emit(Event::info(EventType::REQUEST_RECEIVED, "recv", fd));
    assert(recv.is_ok());

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
