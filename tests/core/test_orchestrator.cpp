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
    // FakeSink sink;
    std::shared_ptr<FakeSink> sink = std::make_shared<FakeSink>();

    orch.attach(sink);

    const int fd = 3;
    const SessionId sid = 11451;

    auto dns_start_e = Event::info(EventType::DNS_RESOLVE_START, "dns start", {fd});
    dns_start_e.session_id = sid;
    auto dns_start = orch.emit(dns_start_e);
    assert(dns_start.is_ok());

    auto dns_done_e = Event::info(EventType::DNS_RESOLVE_DONE, "dns done", {fd});
    dns_done_e.session_id = sid;
    auto dns_done = orch.emit(dns_done_e);
    assert(dns_done.is_ok());

    auto connect_e = Event::info(EventType::TCP_CONNECT_START, "connect", {fd});
    connect_e.session_id = sid;
    auto connect = orch.emit(connect_e);
    assert(connect.is_ok());

    auto established_e = Event::info(EventType::TCP_CONNECT_SUCCESS, "established", {fd});
    established_e.session_id = sid;
    auto established = orch.emit(established_e);
    assert(established.is_ok());

    auto send_e = Event::info(EventType::HTTP_SENT, "send", {fd});
    send_e.session_id = sid;
    auto send = orch.emit(send_e);
    assert(send.is_ok());

    auto recv_e = Event::info(EventType::HTTP_RECEIVED, "recv", {fd});
    recv_e.session_id = sid;
    auto recv = orch.emit(recv_e);
    assert(recv.is_ok());

    // ---------- Sink call count ----------
    assert(sink->records.size() == 6);

    // ---------- FSM state progression ----------
    assert(sink->records[0].state == LifeState::Resolving);
    assert(sink->records[1].state == LifeState::Resolving || sink->records[1].state == LifeState::Connecting);

    assert(sink->records[2].state == LifeState::Connecting);
    assert(sink->records[3].state == LifeState::Established);
    assert(sink->records[4].state == LifeState::Sending);
    assert(sink->records[5].state == LifeState::Receiving || sink->records[5].state == LifeState::Finished);

    // ---------- Timeline consistency ----------
    const auto &timeline = orch.get_timeline();
    assert(timeline.size() == 6);

    auto by_fd = timeline.query_by_fd(fd);
    assert(by_fd.size() == 6);

    // ---------- FSM final state ----------
    const auto *fsm = orch.get_fsm(sid);
    assert(fsm != nullptr);
    assert(fsm->current_state() == LifeState::Finished || fsm->current_state() == LifeState::Receiving);

    std::cout << "[OK] Orchestrator basic flow test passed.\n";
    return 0;
}
