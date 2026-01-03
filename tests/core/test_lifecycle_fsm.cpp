#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

#include "eunet/core/lifecycle_fsm.hpp"
#include "eunet/core/event.hpp"
#include "eunet/platform/time.hpp"

using namespace core;

/* -------------------------------------------------
 * 工具函数
 * -------------------------------------------------*/

static Event make_ok(
    EventType type,
    int fd)
{
    return Event::info(type, "ok", {fd});
}

static Event make_error(
    EventType type,
    int fd,
    const char *msg = "error")
{
    return Event::failure(
        type,
        util::Error::system().message(msg).build(),
        {fd});
}

/* -------------------------------------------------
 * 单 FSM 测试
 * -------------------------------------------------*/

void test_fsm_normal_flow()
{
    LifecycleFSM fsm;

    // DNS
    fsm.on_event(make_ok(EventType::DNS_RESOLVE_START, 3));
    assert(fsm.current_fd() == 3);
    assert(fsm.current_state() == LifeState::Resolving);

    fsm.on_event(make_ok(EventType::DNS_RESOLVE_DONE, 3));
    assert(fsm.current_state() == LifeState::Connecting);

    // TCP
    fsm.on_event(make_ok(EventType::TCP_CONNECT_SUCCESS, 3));
    assert(fsm.current_state() == LifeState::Established);

    // HTTP send
    fsm.on_event(make_ok(EventType::HTTP_REQUEST_BUILD, 3));
    assert(fsm.current_state() == LifeState::Sending);

    fsm.on_event(make_ok(EventType::HTTP_SENT, 3));
    assert(fsm.current_state() == LifeState::Receiving);

    // HTTP recv
    fsm.on_event(make_ok(EventType::HTTP_HEADERS_RECEIVED, 3));
    assert(fsm.current_state() == LifeState::Receiving);

    fsm.on_event(make_ok(EventType::HTTP_BODY_DONE, 3));
    assert(fsm.current_state() == LifeState::Finished);

    assert(!fsm.has_error());
}

void test_fsm_skip_dns()
{
    LifecycleFSM fsm;

    fsm.on_event(make_ok(EventType::TCP_CONNECT_START, 4));
    assert(fsm.current_state() == LifeState::Connecting);

    fsm.on_event(make_ok(EventType::TCP_CONNECT_SUCCESS, 4));
    assert(fsm.current_state() == LifeState::Established);
}

void test_fsm_error_interrupt()
{
    LifecycleFSM fsm;

    fsm.on_event(make_ok(EventType::DNS_RESOLVE_START, 5));
    assert(fsm.current_state() == LifeState::Resolving);

    fsm.on_event(make_error(EventType::DNS_RESOLVE_DONE, 5, "dns failed"));
    assert(fsm.current_state() == LifeState::Error);
    assert(fsm.has_error());

    auto err = fsm.get_last_error();
    assert(err);
    assert(err.domain() == util::ErrorDomain::System);
    assert(err.message() == "dns failed");
}

void test_fsm_error_is_terminal()
{
    LifecycleFSM fsm;

    fsm.on_event(make_error(EventType::TCP_CONNECT_START, 6));
    assert(fsm.current_state() == LifeState::Error);

    fsm.on_event(make_ok(EventType::TCP_CONNECT_SUCCESS, 6));
    assert(fsm.current_state() == LifeState::Error);

    fsm.on_event(make_ok(EventType::HTTP_BODY_DONE, 6));
    assert(fsm.current_state() == LifeState::Error);
}

void test_fsm_timestamp_behavior()
{
    LifecycleFSM fsm;

    auto t0 = platform::time::wall_now();
    fsm.on_event(make_ok(EventType::DNS_RESOLVE_START, 7));

    auto start = fsm.start_timestamp();
    auto last = fsm.last_timestamp();

    assert(start == last);
    assert(start >= t0);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    fsm.on_event(make_ok(EventType::DNS_RESOLVE_DONE, 7));
    assert(fsm.last_timestamp() > start);
}

/* -------------------------------------------------
 * FsmManager 测试
 * -------------------------------------------------*/

void test_manager_basic()
{
    FsmManager mgr;

    assert(mgr.size() == 0);

    Event e = make_ok(EventType::DNS_RESOLVE_START, 10);
    e.session_id = 12345;
    mgr.on_event(e);

    assert(mgr.size() == 1);
    assert(mgr.has(e.session_id));

    const LifecycleFSM *fsm = mgr.get(e.session_id);
    assert(fsm);
    assert(fsm->current_state() == LifeState::Resolving);
}

void test_manager_multi_session()
{
    FsmManager mgr;

    Event e1 = make_ok(EventType::DNS_RESOLVE_START, 1);
    e1.session_id = 12321;

    Event e2 = make_ok(EventType::DNS_RESOLVE_START, 2);
    e2.session_id = 12322;

    Event e3 = make_ok(EventType::DNS_RESOLVE_START, 3);
    e3.session_id = 12323;

    mgr.on_event(e1);
    mgr.on_event(e2);
    mgr.on_event(e3);

    assert(mgr.size() == 3);
    assert(mgr.has(12321));
    assert(mgr.has(12322));
    assert(mgr.has(12323));
}

void test_manager_error_fsm_persist()
{
    FsmManager mgr;

    Event e = make_error(EventType::TCP_CONNECT_START, 20);
    e.session_id = 11111;
    mgr.on_event(e);

    const LifecycleFSM *fsm = mgr.get(e.session_id);
    assert(fsm);
    assert(fsm->current_state() == LifeState::Error);
    assert(fsm->has_error());
}

/* -------------------------------------------------
 * main
 * -------------------------------------------------*/

int main()
{
    test_fsm_normal_flow();
    test_fsm_skip_dns();
    test_fsm_error_interrupt();
    test_fsm_error_is_terminal();
    test_fsm_timestamp_behavior();

    test_manager_basic();
    test_manager_multi_session();
    test_manager_error_fsm_persist();

    std::cout << "[LifecycleFSM] all tests passed\n";
    return 0;
}
