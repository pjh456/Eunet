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
    auto sink = std::make_shared<FakeSink>();
    orch.attach(sink);

    const int fd = 3;
    const SessionId sid = 11451;

    // ---------- DNS ----------
    {
        auto e = Event::info(EventType::DNS_RESOLVE_START, "dns start", {fd});
        e.session_id = sid;
        assert(orch.emit(e).is_ok());
    }

    {
        auto e = Event::info(EventType::DNS_RESOLVE_DONE, "dns done", {fd});
        e.session_id = sid;
        assert(orch.emit(e).is_ok());
    }

    // ---------- TCP ----------
    {
        auto e = Event::info(EventType::TCP_CONNECT_START, "connect", {fd});
        e.session_id = sid;
        assert(orch.emit(e).is_ok());
    }

    {
        auto e = Event::info(EventType::TCP_CONNECT_SUCCESS, "established", {fd});
        e.session_id = sid;
        assert(orch.emit(e).is_ok());
    }

    // ---------- HTTP ----------
    {
        auto e = Event::info(EventType::HTTP_REQUEST_BUILD, "build", {fd});
        e.session_id = sid;
        assert(orch.emit(e).is_ok());
    }

    {
        auto e = Event::info(EventType::HTTP_SENT, "send", {fd});
        e.session_id = sid;
        assert(orch.emit(e).is_ok());
    }

    {
        auto e = Event::info(EventType::HTTP_HEADERS_RECEIVED, "headers", {fd});
        e.session_id = sid;
        assert(orch.emit(e).is_ok());
    }

    {
        auto e = Event::info(EventType::HTTP_BODY_DONE, "body done", {fd});
        e.session_id = sid;
        assert(orch.emit(e).is_ok());
    }

    // ---------- Sink assertions ----------
    assert(sink->records.size() == 8);

    assert(sink->records[0].state == LifeState::Resolving);
    assert(sink->records[1].state == LifeState::Connecting);

    assert(sink->records[2].state == LifeState::Connecting);
    assert(sink->records[3].state == LifeState::Established);

    assert(sink->records[4].state == LifeState::Sending);
    assert(sink->records[5].state == LifeState::Receiving);
    assert(sink->records[6].state == LifeState::Receiving);
    assert(sink->records[7].state == LifeState::Finished);

    // ---------- Timeline ----------
    const auto &timeline = orch.get_timeline();
    assert(timeline.size() == 8);

    auto by_fd = timeline.query_by_fd(fd);
    assert(by_fd.size() == 8);

    // ---------- FSM final state ----------
    const auto *fsm = orch.get_fsm(sid);
    assert(fsm != nullptr);
    assert(fsm->current_state() == LifeState::Finished);
    assert(!fsm->has_error());

    std::cout << "[OK] Orchestrator lifecycle test passed.\n";
    return 0;
}
