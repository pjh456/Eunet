#include "eunet/platform/poller.hpp"

#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

namespace platform::poller
{
    SysResult<Poller> Poller::create()
    {
        Poller p;
        if (!p.epoll_fd.valid())
            return SysResult<Poller>::Err(
                SysError::from_errno(errno));

        return SysResult<Poller>::Ok(std::move(p));
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

    util::Result<bool, PollerError>
    Poller::add(int fd, std::uint32_t events) noexcept
    {
        using Result = util::Result<bool, PollerError>;

        if (!valid())
        {
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::NotInitialized,
                    .cause = SysError{},
                });
        }

        if (fd < 0)
            return Result::Err(
                PollerError{
                    .code = PollerErrorCode::InvalidFd,
                    .cause = SysError{},
                });

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_ADD,
                fd, &ev) == 0)
            return Result::Ok(true);

        const SysError sys = SysError::from_errno(errno);

        PollerErrorCode code = PollerErrorCode::InvalidFd;
        if (errno == EEXIST)
            code = PollerErrorCode::AlreadyExists;
        else if (errno == ENOENT)
            code = PollerErrorCode::NotFound;

        return Result::Err(
            PollerError{
                .code = code,
                .cause = sys,
            });
    }

    util::Result<bool, PollerError>
    Poller::modify(int fd, std::uint32_t events) noexcept
    {
        using Result = util::Result<bool, PollerError>;

        if (!valid())
            return Result::Err(
                PollerError{
                    PollerErrorCode::NotInitialized,
                    SysError{},
                });

        if (fd < 0)
            return Result::Err(
                PollerError{
                    PollerErrorCode::InvalidFd,
                    SysError{},
                });

        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_MOD,
                fd, &ev) == 0)
            return Result::Ok(true);

        SysError sys = SysError::from_errno(errno);

        PollerErrorCode code = PollerErrorCode::InvalidFd;
        if (errno == ENOENT)
            code = PollerErrorCode::NotFound;

        return Result::Err(
            PollerError{
                code,
                sys,
            });
    }

    util::Result<bool, PollerError>
    Poller::remove(int fd) noexcept
    {
        using Result = util::Result<bool, PollerError>;

        if (!valid())
            return Result::Err(
                PollerError{
                    PollerErrorCode::NotInitialized,
                    SysError{},
                });

        if (fd < 0)
            return Result::Err(
                PollerError{
                    PollerErrorCode::InvalidFd,
                    SysError{},
                });

        if (::epoll_ctl(
                epoll_fd.get(),
                EPOLL_CTL_DEL,
                fd, nullptr) == 0)
            return Result::Ok(true);

        SysError sys = SysError::from_errno(errno);

        PollerErrorCode code =
            (errno == ENOENT)
                ? PollerErrorCode::NotFound
                : PollerErrorCode::InvalidFd;

        return Result::Err(
            PollerError{
                code,
                sys,
            });
    }

    util::Result<std::vector<PollEvent>, SysError>
    Poller::wait(int timeout_ms) noexcept
    {
        using Result = util::Result<std::vector<PollEvent>, SysError>;

        if (!valid())
            return Result::Err(SysError::from_errno(EBADF));

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
            return Result::Err(SysError::from_errno(errno));

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

const char *to_string(const platform::poller::PollerErrorCode &code)
{
    using namespace platform::poller;
    switch (code)
    {
    case PollerErrorCode::NotInitialized:
        return "not initialized";
    case PollerErrorCode::InvalidFd:
        return "invalid fd";
    case PollerErrorCode::AlreadyExists:
        return "already exists";
    case PollerErrorCode::NotFound:
        return "not found";
    default:
        return "unknown";
    }
}

std::string format_error(const platform::poller::PollerError &e)
{
    std::string out = "[poller] ";
    out += to_string(e.code);

    if (!e.cause.is_ok())
    {
        out += " | ";
        out += format_error(e.cause);
    }

    return out;
}