#include "eunet/platform/poller.hpp"

#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

namespace platform::poller
{
    Poller::Poller() : epoll_fd(::epoll_create1(EPOLL_CLOEXEC)) {}

    bool Poller::valid() const noexcept { return epoll_fd.valid(); }

    bool Poller::add(int fd, std::uint32_t events) noexcept
    {
        if (!valid() || fd < 0)
            return false;

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;

        return ::epoll_ctl(
                   epoll_fd.get(), EPOLL_CTL_ADD,
                   fd, &ev) == 0;
    }

    bool Poller::modify(int fd, std::uint32_t events) noexcept
    {
        if (!valid() || fd < 0)
            return false;

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;

        return ::epoll_ctl(
                   epoll_fd.get(), EPOLL_CTL_MOD,
                   fd, &ev) == 0;
    }

    void Poller::remove(int fd) noexcept
    {
        if (!valid() || fd < 0)
            return;

        ::epoll_ctl(
            epoll_fd.get(), EPOLL_CTL_DEL,
            fd, nullptr);
    }

    std::vector<PollEvent> Poller::wait(int timeout_ms)
    {
        std::vector<PollEvent> result;

        if (!valid())
            return result;

        epoll_event events[MAX_EVENTS];
        int n;
        do
        {
            n = ::epoll_wait(
                epoll_fd.get(), events,
                MAX_EVENTS, timeout_ms);
        } while (n < 0 && errno == EINTR);

        if (n <= 0)
            return result;

        result.reserve(static_cast<size_t>(n));

        for (int i = 0; i < n; ++i)
        {
            PollEvent ev;
            ev.fd = events[i].data.fd;
            ev.events = events[i].events;
            result.push_back(ev);
        }

        return result;
    }
}