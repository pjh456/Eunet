#include <iostream>
#include <thread>
#include <chrono>
#include "eunet/core/timeline.hpp"

using namespace core;

void test_timeline()
{
    Timeline tl;

    // 1. 创建几个事件
    auto now = platform::time::wall_now();

    Event e1 = Event::create(EventType::DNS_START, Event::MsgResult::Ok("DNS lookup start"), 3);
    Event e2 = Event::create(EventType::TCP_CONNECT, Event::MsgResult::Ok("TCP connecting"), 3);
    Event e3 = Event::create(EventType::REQUEST_SENT, Event::MsgResult::Ok("Request sent"), 4);
    Event e4 = Event::create(EventType::ERROR, Event::MsgResult::Err(Error{100, "Timeout"}), 3);

    // 2. 推入事件（单个）
    auto res1 = tl.push(e1);
    if (!res1.is_ok())
        std::cerr << "Push failed: " << res1.unwrap_err().message << "\n";

    // 3. 推入事件（批量）
    std::vector<Event> batch = {e2, e3, e4};
    auto res2 = tl.push(batch);
    if (!res2.is_ok())
        std::cerr << "Batch push failed: " << res2.unwrap_err().message << "\n";

    std::cout << "Total events after push: " << tl.size() << "\n";

    // 4. 查询数量
    std::cout << "Count by fd 3: " << tl.count_by_fd(3) << "\n";
    std::cout << "Count by type ERROR: " << tl.count_by_type(EventType::ERROR) << "\n";

    // 5. 查询是否包含类型
    std::cout << "Has TCP_CONNECT: " << tl.has_type(EventType::TCP_CONNECT) << "\n";

    // 6. query_by_fd
    auto qfd = tl.query_by_fd(3);
    if (qfd.is_ok())
    {
        std::cout << "Events for fd 3:\n";
        for (auto ev : qfd.unwrap())
            std::cout << "  " << ev->to_string() << "\n";
    }

    // 7. query_by_type
    auto qtype = tl.query_by_type(EventType::ERROR);
    if (qtype.is_ok())
    {
        std::cout << "ERROR events:\n";
        for (auto ev : qtype.unwrap())
            std::cout << "  " << ev->to_string() << "\n";
    }

    // 8. query_by_time
    auto start = now - std::chrono::seconds(1);
    auto end = now + std::chrono::seconds(10);
    auto qtime = tl.query_by_time(start, end);
    if (qtime.is_ok())
        std::cout << "Events in time window: " << qtime.unwrap().size() << "\n";

    // 9. latest_event
    auto latest = tl.latest_event();
    if (latest.is_ok())
        std::cout << "Latest event: " << latest.unwrap()->to_string() << "\n";

    // 10. latest_by_fd
    auto latest_fd3 = tl.latest_by_fd(3);
    if (latest_fd3.is_ok())
        std::cout << "Latest event for fd 3: " << latest_fd3.unwrap()->to_string() << "\n";

    // 11. replay_all
    auto replay_all = tl.replay_all();
    if (replay_all.is_ok())
    {
        std::cout << "Replay all events:\n";
        for (auto ev : replay_all.unwrap())
            std::cout << "  " << ev->to_string() << "\n";
    }

    // 12. replay_by_fd
    auto replay_fd3 = tl.replay_by_fd(3);
    if (replay_fd3.is_ok())
    {
        std::cout << "Replay fd 3 events:\n";
        for (auto ev : replay_fd3.unwrap())
            std::cout << "  " << ev->to_string() << "\n";
    }

    // 13. replay_since
    auto replay_since = tl.replay_since(now);
    if (replay_since.is_ok())
    {
        std::cout << "Replay events since now:\n";
        for (auto ev : replay_since.unwrap())
            std::cout << "  " << ev->to_string() << "\n";
    }

    // 14. remove_by_fd
    auto removed_fd = tl.remove_by_fd(4);
    if (removed_fd.is_ok())
        std::cout << "Removed events by fd 4: " << removed_fd.unwrap() << "\n";

    // 15. remove_by_type
    auto removed_type = tl.remove_by_type(EventType::ERROR);
    if (removed_type.is_ok())
        std::cout << "Removed ERROR events: " << removed_type.unwrap() << "\n";

    // 16. remove_by_time
    auto removed_time = tl.remove_by_time(start, end);
    if (removed_time.is_ok())
        std::cout << "Removed events in time window: " << removed_time.unwrap() << "\n";

    // 17. clear
    tl.clear();
    std::cout << "Total events after clear: " << tl.size() << "\n";

    // 18. 测试无序事件和 sort_by_time
    Event e5 = Event::create(EventType::TCP_ESTABLISHED, Event::MsgResult::Ok("Conn established"), 5);
    Event e6 = Event::create(EventType::REQUEST_RECEIVED, Event::MsgResult::Ok("Request received"), 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 确保时间不同
    Event e7 = Event::create(EventType::REQUEST_SENT, Event::MsgResult::Ok("Request sent later"), 5);

    (void)tl.push({e7, e5, e6}); // 故意乱序
    std::cout << "Before sorting:\n";
    for (auto ev : tl.all_events())
        std::cout << "  " << ev.to_string() << "\n";

    auto sort_res = tl.sort_by_time();
    if (sort_res.is_ok())
    {
        std::cout << "After sorting:\n";
        for (auto ev : tl.all_events())
            std::cout << "  " << ev.to_string() << "\n";
    }
    else
    {
        std::cerr << "Sort failed: " << sort_res.unwrap_err().message << "\n";
    }
}

int main()
{
    test_timeline();
    return 0;
}
