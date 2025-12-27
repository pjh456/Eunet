#include <iostream>
#include <cassert>
#include <string>
#include "eunet/core/event.hpp"

void test_event()
{
    using namespace core;

    std::cout << "=== Event Unit Test Start ===\n";

    // 1. 创建简单事件
    auto e1 = Event::create(EventType::DNS_START);
    assert(e1.type == EventType::DNS_START);
    assert(e1.fd == -1);
    assert(e1.msg.is_ok());
    assert(e1.msg.unwrap() == "");
    std::cout << "e1: " << e1.to_string() << "\n";

    // 2. 创建带消息事件
    auto e2 = Event::create(EventType::REQUEST_SENT, Event::MsgResult::Ok("HTTP GET"));
    assert(e2.type == EventType::REQUEST_SENT);
    assert(e2.msg.is_ok() && e2.msg.unwrap() == "HTTP GET");
    std::cout << "e2: " << e2.to_string() << "\n";

    // 3. 创建带错误事件
    Error err{404, "Not Found"};
    auto e3 = Event::create(EventType::ERROR, Event::MsgResult::Err(err));
    assert(e3.type == EventType::ERROR);
    assert(!e3.msg.is_ok());
    assert(e3.msg.unwrap_err().code == 404);
    std::cout << "e3: " << e3.to_string() << "\n";

    // 4. 创建带 fd 和 data 的事件
    int fd_test = 10;
    std::string payload = "Response body";
    auto e4 = Event::create(EventType::REQUEST_RECEIVED, Event::MsgResult::Ok("OK"), fd_test, payload);
    assert(e4.fd == fd_test);
    assert(std::any_cast<std::string>(e4.data) == payload);
    std::cout << "e4: " << e4.to_string() << "\n";

    // 5. 时间戳检查（非严格，确保非零即可）
    assert(e1.ts.time_since_epoch().count() > 0);
    assert(e2.ts.time_since_epoch().count() > 0);

    std::cout << "=== Event Unit Test Passed ===\n";
}

int main()
{
    test_event();
    return 0;
}
