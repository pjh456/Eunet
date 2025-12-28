#include <cassert>
#include <thread>
#include <chrono>
#include <string>

#include "eunet/core/timeline.hpp"

using namespace core;

void test_timeline()
{
    Timeline tl;

    // 1. 创建事件
    auto now = platform::time::wall_now();

    Event e1 = Event::info(
        EventType::DNS_RESOLVE_START,
        "DNS lookup start",
        {3});

    Event e2 = Event::info(
        EventType::TCP_CONNECT_START,
        "TCP connecting",
        {3});

    Event e3 = Event::info(
        EventType::HTTP_SENT,
        "Request sent",
        {4});

    Event e4 = Event::failure(
        EventType::HTTP_RECEIVED,
        util::Error::from_gai(0, "timeout"),
        {3});

    // 2. push 单个
    {
        auto res = tl.push(e1);
        assert(res.is_ok());
        assert(tl.size() == 1);
    }

    // 3. push 批量
    {
        auto res = tl.push({e2, e3, e4});
        assert(res.is_ok());
        assert(tl.size() == 4);
    }

    // 4. count / has
    assert(tl.count_by_fd(3) == 3);
    assert(tl.count_by_fd(4) == 1);

    assert(tl.count_by_type(EventType::DNS_RESOLVE_START) == 1);
    assert(tl.count_by_type(EventType::TCP_CONNECT_START) == 1);

    assert(tl.has_type(EventType::TCP_CONNECT_START));
    assert(!tl.has_type(EventType::TCP_CONNECT_SUCCESS));

    // 5. query_by_fd
    {
        auto list = tl.query_by_fd(3);
        assert(list.size() == 3);
        for (auto ev : list)
            assert(ev->fd.fd == 3);
    }

    // 6. query_by_type
    {
        auto list = tl.query_by_type(EventType::HTTP_RECEIVED);
        assert(list.size() == 1);
        assert(list.front()->error);
    }

    // 7. query_errors
    {
        auto errs = tl.query_errors();
        assert(errs.size() == 1);
        assert(errs.front()->error);
        assert(errs.front()->error.domain() == util::ErrorDomain::Network);
    }

    // 8. query_by_time
    {
        auto start = now - std::chrono::seconds(1);
        auto end = now + std::chrono::seconds(10);

        auto list = tl.query_by_time(start, end);
        assert(list.size() == tl.size());
    }

    // 9. latest_event
    {
        auto latest = tl.latest_event();
        assert(latest.is_ok());
        assert(latest.unwrap()->error);
    }

    // 10. latest_by_fd
    {
        auto latest = tl.latest_by_fd(3);
        assert(latest.is_ok());
        assert(latest.unwrap()->fd.fd == 3);
    }

    // 11. replay_all
    {
        auto list = tl.replay_all();
        assert(list.size() == tl.size());
    }

    // 12. replay_by_fd
    {
        auto list = tl.replay_by_fd(3);
        assert(list.size() == 3);
        for (auto ev : list)
            assert(ev->fd.fd == 3);
    }

    // 13. replay_since
    {
        auto list = tl.replay_since(now);
        assert(list.size() == tl.size());
    }

    // 14. remove_by_fd
    {
        auto removed = tl.remove_by_fd(4);
        assert(removed == 1);
        assert(tl.count_by_fd(4) == 0);
        assert(tl.size() == 3);
    }

    // 15. remove_by_type
    {
        auto removed = tl.remove_by_type(EventType::HTTP_RECEIVED);
        assert(removed == 1);
        assert(tl.query_errors().empty());
        assert(tl.size() == 2);
    }

    // 16. remove_by_time（非法区间 → 0）
    {
        auto start = now + std::chrono::seconds(10);
        auto end = now - std::chrono::seconds(10);

        auto removed = tl.remove_by_time(start, end);
        assert(removed == 0);
        assert(tl.size() == 2);
    }

    // 17. clear
    {
        tl.clear();
        assert(tl.size() == 0);
    }

    // 18. 无序事件 + sort_by_time
    {
        Event e5 = Event::info(
            EventType::TCP_CONNECT_SUCCESS,
            "Conn established",
            {5});

        Event e6 = Event::info(
            EventType::HTTP_RECEIVED,
            "Request received",
            {5});

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        Event e7 = Event::info(
            EventType::HTTP_SENT,
            "Request sent later",
            {5});

        auto res = tl.push({e7, e5, e6}); // 故意乱序
        assert(res.is_ok());
        assert(tl.size() == 3);

        auto sort_res = tl.sort_by_time();
        assert(sort_res.is_ok());

        auto all = tl.replay_all();
        for (size_t i = 1; i < all.size(); ++i)
            assert(all[i - 1]->ts <= all[i]->ts);
    }
}

int main()
{
    test_timeline();
    return 0;
}
