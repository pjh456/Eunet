#include <iostream>
#include <cassert>
#include <string>
#include <any>

#include "eunet/core/event.hpp"

using namespace core;

void test_event()
{
    std::cout << "=== Event Unit Test Start ===\n";

    // 1. info 事件（最简单）
    auto e1 = Event::info(EventType::DNS_RESOLVE_START, "");
    assert(e1.type == EventType::DNS_RESOLVE_START);
    assert(!e1.fd);
    assert(e1.is_ok());
    assert(!e1.is_error());
    assert(e1.msg.empty());
    assert(!e1.error);
    std::cout << "e1: " << to_string(e1) << "\n";

    // 2. info 事件（带消息）
    auto e2 = Event::info(EventType::HTTP_SENT, "HTTP GET");
    assert(e2.type == EventType::HTTP_SENT);
    assert(e2.is_ok());
    assert(e2.msg == "HTTP GET");
    std::cout << "e2: " << to_string(e2) << "\n";

    // 3. failure 事件
    auto err =
        util::Error::protocol()
            .message("404 Not Found")
            .build();

    auto e3 = Event::failure(EventType::HTTP_RECEIVED, err);
    assert(e3.type == EventType::HTTP_RECEIVED);
    assert(e3.is_error());
    assert(!e3.is_ok());
    assert(e3.error);
    std::cout << e3.error->message() << std::endl;
    assert(e3.error->message() == "404 Not Found");
    std::cout << "e3: " << to_string(e3) << "\n";

    // 4. 带 fd 与 data 的 info 事件
    int fd_test = 10;
    std::string payload = "Response body";

    auto e4 = Event::info(
        EventType::HTTP_RECEIVED,
        "OK",
        {fd_test});

    assert(e4.fd.fd == fd_test);
    assert(e4.is_ok());
    std::cout << "e4: " << to_string(e4) << "\n";

    // 5. 带 fd 与 data 的 failure 事件
    auto e5 = Event::failure(
        EventType::TCP_CONNECT_START,
        util::Error::transport().connection_refused().build(),
        {fd_test});

    assert(e5.is_error());
    assert(e5.fd.fd == fd_test);
    // assert(std::any_cast<int>(e5.data) == 12345);
    std::cout << "e5: " << to_string(e5) << "\n";

    // 6. 时间戳存在性检查（非 0 即可）
    assert(e1.ts.time_since_epoch().count() > 0);
    assert(e3.ts.time_since_epoch().count() > 0);

    std::cout << "=== Event Unit Test Passed ===\n";
}

int main()
{
    test_event();
    return 0;
}
