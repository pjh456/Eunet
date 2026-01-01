#include "eunet/platform/base_socket.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <utility>

namespace platform::net
{
    BaseSocket::BaseSocket(fd::Fd &&fd)
        : m_fd(std::move(fd)) {}

    fd::FdView
    BaseSocket::view()
        const noexcept { return m_fd.view(); }

    bool BaseSocket::is_open()
        const noexcept { return (bool)m_fd.view(); }

    void BaseSocket::close() noexcept { m_fd.reset(-1); }

    util::ResultV<Endpoint>
    BaseSocket::local_endpoint() const
    {
        using Result = util::ResultV<Endpoint>;
        using util::Error;

        sockaddr_storage addr{};
        socklen_t len = sizeof(addr);

        if (::getsockname(
                view().fd,
                reinterpret_cast<sockaddr *>(&addr),
                &len) < 0)
        {
            int err_no = errno;
            return Result::Err(
                Error::system()
                    .code(err_no)
                    .message("getsockname failed")
                    .build());
        }

        return Result::Ok(
            Endpoint(
                reinterpret_cast<sockaddr *>(&addr),
                static_cast<socklen_t>(len)));
    }

    util::ResultV<Endpoint>
    BaseSocket::remote_endpoint() const
    {
        using Result = util::ResultV<Endpoint>;
        using util::Error;

        sockaddr_storage addr{};
        socklen_t len = sizeof(addr);

        if (::getpeername(
                view().fd,
                reinterpret_cast<sockaddr *>(&addr),
                &len) < 0)
        {
            int err_no = errno;
            return Result::Err(
                Error::system()
                    .code(err_no)
                    .message("getpeername failed")
                    .build());
        }

        return Result::Ok(
            Endpoint(
                reinterpret_cast<sockaddr *>(&addr),
                static_cast<socklen_t>(len)));
    }

    util::ResultV<void>
    wait_fd_epoll(
        poller::Poller &poller,
        fd::FdView fd,
        uint32_t events,
        int timeout_ms)
    {
        using Result = util::ResultV<void>;
        using util::Error;

        auto r = poller.add(fd, events);
        if (r.is_err())
            return Result::Err(r.unwrap_err());

        auto evs = poller.wait(timeout_ms);
        poller.remove(fd); // 一定要清理

        if (evs.is_err())
            return Result::Err(evs.unwrap_err());

        if (evs.unwrap().empty())
        {
            return Result::Err(
                Error::transport()
                    .timeout()
                    .message("epoll wait timeout")
                    .build());
        }

        for (auto &ev : evs.unwrap())
        {
            if (ev.fd != fd)
                continue;

            if (ev.events & (EPOLLERR | EPOLLHUP))
            {
                return Result::Err(
                    Error::transport()
                        .message("socket error or hangup")
                        .build());
            }

            if (ev.events & events)
                return Result::Ok();
        }

        return Result::Err(
            Error::transport()
                .message("unexpected epoll event")
                .build());
    }

}
