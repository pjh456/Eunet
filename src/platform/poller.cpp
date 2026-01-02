#include "eunet/platform/poller.hpp"

#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

namespace platform::poller
{
    util::ResultV<Poller> Poller::create()
    {
        using Ret = util::ResultV<Poller>;
        using util::Error;
        Poller p;
        if (!p.epoll_fd.valid())
        {
            int err_no = errno;
            return Ret::Err(
                Error::system()
                    .code(err_no)
                    .set_category(from_errno(err_no)) // 主要是 ResourceExhausted
                    .message("Failed to create epoll instance")
                    .context("epoll_create1")
                    .build());
        }

        return Ret::Ok(std::move(p));
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

    bool Poller::has_fd(int fd) const noexcept { return fd_table.count(fd); }

    util::ResultV<void>
    Poller::add(
        platform::fd::FdView fd,
        std::uint32_t events) noexcept
    {
        if (has_fd(fd.fd))
            return modify(fd, events);
        fd_table.emplace(fd.fd);

        using Ret = util::ResultV<void>;
        using util::Error;

        if (!valid())
        {
            return Ret::Err(
                Error::internal()
                    .invalid_argument()
                    .message("Poller is not initialized")
                    .build());
        }

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd.fd;

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_ADD,
                fd.fd, &ev) == 0)
            return Ret::Ok();

        int err_no = errno;
        return Ret::Err(
            Error::system()
                .code(err_no)
                .set_category(from_errno(err_no))
                .message("Failed to update epoll interest list")
                .context("Poller.add: epoll_ctl")
                .build());
    }

    util::ResultV<void>
    Poller::modify(
        platform::fd::FdView fd,
        std::uint32_t events) noexcept
    {
        if (!has_fd(fd.fd))
            return add(fd, events);

        using Ret = util::ResultV<void>;
        using util::Error;

        if (!valid())
        {
            return Ret::Err(
                Error::internal()
                    .invalid_argument()
                    .message("Poller is not initialized")
                    .build());
        }

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd.fd;

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_MOD,
                fd.fd, &ev) == 0)
            return Ret::Ok();

        int err_no = errno;
        return Ret::Err(
            Error::system()
                .code(err_no)
                .set_category(from_errno(err_no))
                .message("Failed to update epoll interest list")
                .context("Poller.modify: epoll_ctl")
                .build());
    }

    util::ResultV<void>
    Poller::remove(
        platform::fd::FdView fd) noexcept
    {
        using Ret = util::ResultV<void>;
        using util::Error;

        if (!valid())
        {
            return Ret::Err(
                Error::internal()
                    .invalid_argument()
                    .message("Poller is not initialized")
                    .build());
        }

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_DEL,
                fd.fd, nullptr) == 0)
        {
            fd_table.erase(fd.fd);
            return Ret::Ok();
        }

        int err_no = errno;
        return Ret::Err(
            Error::system()
                .code(err_no)
                .set_category(from_errno(err_no))
                .message("Failed to update epoll interest list")
                .context("Poller.remove: epoll_ctl")
                .build());
    }

    util::ResultV<std::vector<PollEvent>>
    Poller::wait(int timeout_ms) noexcept
    {
        using Ret = util::ResultV<std::vector<PollEvent>>;
        using util::Error;

        if (!valid())
        {
            return Ret::Err(
                Error::internal()
                    .invalid_argument()
                    .message("Poller is not initialized")
                    .build());
        }

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
        {
            int err_no = errno;
            return Ret::Err(
                Error::system()
                    .code(err_no)
                    .set_category(from_errno(err_no)) // 主要是 EINTR (Cancelled)
                    .message("Epoll wait syscall failed")
                    .context("epoll_wait")
                    .build());
        }

        std::vector<PollEvent> result;
        result.reserve(static_cast<size_t>(n));

        for (size_t i = 0; i < n; ++i)
        {
            result.push_back({
                events[i].data.fd,
                events[i].events,
            });
        }

        return Ret::Ok(std::move(result));
    }
}