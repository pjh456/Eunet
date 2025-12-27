#include "eunet/platform/poller.hpp"

#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

namespace platform::poller
{
    util::Result<Poller, PollerError>
    Poller::create()
    {
        using Result = util::Result<Poller, PollerError>;
        Poller p;
        if (!p.epoll_fd.valid())
        {
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::SystemError,
                    .sys_errno = errno,
                    .message = "epoll_create1 failed",
                });
        }
        return Result::Ok(std::move(p));
    }

    Poller::Poller() : epoll_fd(::epoll_create1(EPOLL_CLOEXEC)) {}

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

    util::Result<bool, PollerError>
    Poller::add(int fd, std::uint32_t events) noexcept
    {
        using Result = util::Result<bool, PollerError>;

        if (!valid())
        {
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::NotInitialized,
                    .sys_errno = 0,
                    .message = "Poller not initialized",
                });
        }

        if (fd < 0)
        {
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::InvalidFd,
                    .sys_errno = 0,
                    .message = "Invalid file descriptor",
                });
        }

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_ADD,
                fd, &ev) == 0)
            return Result::Ok(true);

        PollerErrorCode code = PollerErrorCode::SystemError;
        std::string hint;
        switch (errno)
        {
        case EEXIST:
            code = PollerErrorCode::AlreadyExists;
            hint = "Use modify() to update events for this fd";
            break;
        case EBADF:
            code = PollerErrorCode::InvalidFd;
            hint = "Ensure fd is valid and not closed";
            break;
        case EPERM:
            code = PollerErrorCode::PermissionDenied;
            hint = "You might need elevated privileges";
            break;
        case ENOENT:
            code = PollerErrorCode::NotFound;
            hint = "Use add() first to register fd";
            break;
        default:
            hint = "Check system errno for details";
        }

        return Result::Err(
            PollerError{
                .code = code,
                .sys_errno = errno,
                .message = "epoll_ctl ADD failed",
                .hint = hint,
            });
    }

    util::Result<bool, PollerError>
    Poller::modify(int fd, std::uint32_t events) noexcept
    {
        using Result = util::Result<bool, PollerError>;

        if (!valid())
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::NotInitialized,
                    .sys_errno = 0,
                    .message = "Poller not initialized",
                });

        if (fd < 0)
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::InvalidFd,
                    .sys_errno = 0,
                    .message = "Invalid file descriptor",
                });

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_MOD,
                fd, &ev) == 0)
            return Result::Ok(true);

        PollerErrorCode code = PollerErrorCode::SystemError;
        std::string hint;
        switch (errno)
        {
        case EEXIST:
            code = PollerErrorCode::AlreadyExists;
            hint = "Use modify() to update events for this fd";
            break;
        case EBADF:
            code = PollerErrorCode::InvalidFd;
            hint = "Ensure fd is valid and not closed";
            break;
        case EPERM:
            code = PollerErrorCode::PermissionDenied;
            hint = "You might need elevated privileges";
            break;
        case ENOENT:
            code = PollerErrorCode::NotFound;
            hint = "Use add() first to register fd";
            break;
        default:
            hint = "Check system errno for details";
        }

        return Result::Err(
            PollerError{
                .code = code,
                .sys_errno = errno,
                .message = "epoll_ctl MOD failed",
                .hint = hint,
            });
    }

    util::Result<int, PollerError>
    Poller::remove(int fd) noexcept
    {
        using Result = util::Result<int, PollerError>;

        if (!valid())
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::NotInitialized,
                    .sys_errno = 0,
                    .message = "Poller not initialized",
                });

        if (fd < 0)
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::InvalidFd,
                    .sys_errno = 0,
                    .message = "Invalid file descriptor",
                });

        if (::epoll_ctl(
                epoll_fd.get(), EPOLL_CTL_DEL, fd, nullptr) == 0)
            return Result::Ok(0);

        PollerErrorCode code = PollerErrorCode::SystemError;
        std::string hint;
        if (errno == ENOENT)
        {
            code = PollerErrorCode::NotFound;
            hint = "FD was not registered or already removed";
        }
        else
            hint = "System error, check errno";

        return Result::Err(
            PollerError{
                .code = code,
                .sys_errno = errno,
                .message = "epoll_ctl DEL failed",
                .hint = hint,
            });
    }

    util::Result<std::vector<PollEvent>, PollerError>
    Poller::wait(int timeout_ms)
    {
        using Result = util::Result<std::vector<PollEvent>, PollerError>;

        if (!valid())
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::NotInitialized,
                    .sys_errno = 0,
                    .message = "Poller not initialized",
                });

        epoll_event events[MAX_EVENTS];
        int n;

        do
        {
            n = ::epoll_wait(
                epoll_fd.get(), events, MAX_EVENTS, timeout_ms);
        } while (n < 0 && errno == EINTR);

        if (n < 0)
        {
            std::string hint;
            if (errno == EBADF)
                hint = "epoll fd invalid, maybe closed?";
            else if (errno == EINVAL)
                hint = "Bad arguments or max events exceeded";
            else
                hint = "Check system errno";

            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::SystemError,
                    .sys_errno = errno,
                    .message = "epoll_wait failed",
                    .hint = hint,
                });
        }

        std::vector<PollEvent> result;
        result.reserve(static_cast<size_t>(n));

        for (size_t i = 0; i < n; ++i)
        {
            result.push_back(
                PollEvent{
                    .fd = events[i].data.fd,
                    .events = events[i].events,
                });
        }

        return Result::Ok(std::move(result));
    }

}