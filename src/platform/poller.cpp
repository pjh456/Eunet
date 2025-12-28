#include "eunet/platform/poller.hpp"

#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

namespace platform::poller
{
    util::ResultV<Poller> Poller::create()
    {
        Poller p;
        if (!p.epoll_fd.valid())
            return util::ResultV<Poller>::Err(
                util::Error::from_errno(errno));

        return util::ResultV<Poller>::Ok(std::move(p));
    }

    Poller::Poller()
        : epoll_fd(::epoll_create1(EPOLL_CLOEXEC)) {}

    Poller::Poller(Poller &&other) noexcept
        : epoll_fd(std::move(other.epoll_fd)) {}

    Poller &Poller::operator=(Poller &&other) noexcept
    {
        if (this == &other)
            return *this;

        epoll_fd = std::move(other.epoll_fd);
        return *this;
    }

    bool Poller::valid() const noexcept { return epoll_fd.valid(); }

    platform::fd::Fd &Poller::get_fd() noexcept { return epoll_fd; }
    const platform::fd::Fd &Poller::get_fd() const noexcept { return epoll_fd; }

    util::ResultV<void>
    Poller::add(
        const platform::fd::Fd &fd,
        std::uint32_t events) noexcept
    {
        if (!valid())
            return util::ResultV<void>::Err(
                util::Error::internal(
                    "Invalid epoll fd"));

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd.get();

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_ADD,
                fd.get(), &ev) == 0)
            return util::ResultV<void>::Ok();

        return util::ResultV<void>::Err(
            util::Error::from_errno(errno));
    }

    util::ResultV<void>
    Poller::modify(
        const platform::fd::Fd &fd,
        std::uint32_t events) noexcept
    {
        if (!valid())
            return util::ResultV<void>::Err(
                util::Error::internal(
                    "Invalid epoll fd"));

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd.get();

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_MOD,
                fd.get(), &ev) == 0)
            return util::ResultV<void>::Ok();

        return util::ResultV<void>::Err(
            util::Error::from_errno(errno));
    }

    util::ResultV<void>
    Poller::remove(const platform::fd::Fd &fd) noexcept
    {
        if (!valid())
            return util::ResultV<void>::Err(
                util::Error::internal(
                    "Invalid epoll fd"));

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_DEL,
                fd.get(), nullptr) == 0)
            return util::ResultV<void>::Ok();

        return util::ResultV<void>::Err(
            util::Error::from_errno(errno));
    }

    util::ResultV<std::vector<PollEvent>>
    Poller::wait(int timeout_ms) noexcept
    {
        using Result = util::ResultV<std::vector<PollEvent>>;

        if (!valid())
            return Result::Err(util::Error::from_errno(EBADF));

        epoll_event events[MAX_EVENTS];
        int n;

        do
        {
            n = ::epoll_wait(
                epoll_fd.get(),
                events,
                MAX_EVENTS,
                timeout_ms);
        } while (n < 0 && errno == EINTR);

        if (n < 0)
            return Result::Err(util::Error::from_errno(errno));

        std::vector<PollEvent> result;
        result.reserve(static_cast<size_t>(n));

        for (size_t i = 0; i < n; ++i)
        {
            result.push_back(
                {
                    events[i].data.fd,
                    events[i].events,
                });
        }

        return Result::Ok(std::move(result));
    }

}