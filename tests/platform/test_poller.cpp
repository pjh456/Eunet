#include "eunet/platform/fd.hpp"
#include "eunet/platform/poller.hpp"

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>

using platform::fd::Fd;
using platform::poller::Poller;
using platform::poller::PollEvent;

void test_poller_basic()
{
    // 1. 创建 Poller
    auto poller_res = Poller::create();
    assert(poller_res.is_ok());
    Poller poller = std::move(poller_res.unwrap());

    // 2. 创建 pipe
    auto pipe_res = Fd::pipe();
    assert(pipe_res.is_ok());
    auto [read_fd, write_fd] = std::move(pipe_res.unwrap());
    assert(read_fd.valid());
    assert(write_fd.valid());

    int rfd = read_fd.get();
    int wfd = write_fd.get();

    // 3. 注册读端 EPOLLIN
    {
        auto res = poller.add(read_fd.view(), EPOLLIN);
        assert(res.is_ok());
    }

    // 4. 立刻 wait，应当超时返回空
    {
        auto res = poller.wait(10);
        assert(res.is_ok());
        auto events = res.unwrap();
        assert(events.empty());
    }

    // 5. 向写端写入数据，触发读事件
    const char msg[] = "hello";
    ssize_t n = ::write(wfd, msg, sizeof(msg));
    assert(n == sizeof(msg));

    // 6. wait，必须返回一个 EPOLLIN 事件
    {
        auto res = poller.wait(1000);
        assert(res.is_ok());
        auto events = res.unwrap();
        assert(events.size() == 1);

        PollEvent ev = events[0];
        assert(ev.fd.fd == rfd);
        assert(ev.events & EPOLLIN);
    }

    // 7. 读出数据，清空 pipe
    char buf[16];
    ssize_t rn = ::read(rfd, buf, sizeof(buf));
    assert(rn == sizeof(msg));
    assert(std::memcmp(buf, msg, sizeof(msg)) == 0);

    // 8. 再次 wait，不应再有事件
    {
        auto res = poller.wait(10);
        assert(res.is_ok());
        auto events2 = res.unwrap();
        assert(events2.empty());
    }

    // 9. 测试 modify（虽然 pipe 对 OUT 没啥意义，但 API 必须通）
    {
        auto res = poller.modify(read_fd.view(), EPOLLIN | EPOLLET);
        assert(res.is_ok());
    }

    // 10. remove 后，不应再收到事件
    {
        auto res = poller.remove(read_fd.view());
        assert(res.is_ok());
    }

    const char msg2[] = "world";
    ::write(wfd, msg2, sizeof(msg2));

    {
        auto res = poller.wait(50);
        assert(res.is_ok());
        auto events3 = res.unwrap();
        assert(events3.empty());
    }

    // 11. 确认 Poller 没有 close fd（fd 仍然可用）
    ssize_t rn2 = ::read(rfd, buf, sizeof(buf));
    assert(rn2 == sizeof(msg2));
    assert(std::memcmp(buf, msg2, sizeof(msg2)) == 0);

    std::cout << "[test_poller_basic] all assertions passed\n";

    // fd 会在 Fd 析构时自动 close
}

int main()
{
    test_poller_basic();
}
