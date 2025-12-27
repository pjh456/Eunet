#include "eunet//platform/fd.hpp"

#include <cassert>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

#include <utility>

using platform::fd::Fd;

void test_fd_basic()
{
    // 1. 默认构造
    Fd f;
    assert(!f);
    assert(!f.valid());

    // 2. socket 创建
    auto ress = Fd::socket(AF_INET, SOCK_STREAM, 0);
    assert(ress.is_ok());
    Fd s = std::move(ress).unwrap();
    assert(s);
    assert(s.get() >= 0);

    int raw = s.get();

    // 3. move 构造
    Fd moved = std::move(s);
    assert(moved);
    assert(!s);
    assert(moved.get() == raw);

    // 4. release
    int released = moved.release();
    assert(released == raw);
    assert(!moved);

    // 手动 close 防泄漏
    ::close(released);

    // 5. reset
    auto resa = Fd::socket(AF_INET, SOCK_STREAM, 0);
    assert(resa.is_ok());
    auto a = std::move(resa).unwrap();
    int a_fd = a.get();
    a.reset(-1);
    assert(!a);

    // a_fd 已被 close，再 close 应该失败
    assert(::close(a_fd) == -1);

    // 6. pipe
    auto respip = Fd::pipe();
    assert(respip.is_ok());
    auto [r, w] = std::move(respip).unwrap();
    assert(r.get() >= 0 && w.get() >= 0);

    // 7. move assign
    Fd x = std::move(r);
    assert(x);
    assert(!r);
}

int main()
{
    test_fd_basic();
}