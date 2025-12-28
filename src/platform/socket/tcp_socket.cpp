#include "eunet/platform/socket/tcp_socket.hpp"
#include "eunet/platform/poller.hpp"
#include "eunet/platform/time.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <cerrno>

namespace platform::net
{
    util::ResultV<TCPSocket>
    TCPSocket::create(AddressFamily af)
    {
        using Result = util::ResultV<TCPSocket>;

        int domain = (af == AddressFamily::IPv6) ? AF_INET6 : AF_INET;

        auto fd_res =
            fd::Fd::socket(
                domain,
                SOCK_STREAM,
                0);

        if (fd_res.is_err())
            return Result::Err(fd_res.unwrap_err());

        return Result::Ok(
            TCPSocket(std::move(fd_res.unwrap())));
    }

    TCPSocket::TCPSocket(fd::Fd &&fd) noexcept
        : SocketBase(std::move(fd)) {}

    util::ResultV<void>
    TCPSocket::connect(
        const SocketAddress &addr,
        time::Duration timeout)
    {
        using Result = util::ResultV<void>;

        NonBlockingGuard guard(view());

        int ret = ::connect(
            view().fd,
            addr.as_sockaddr(),
            addr.length());

        if (ret == 0)
            return Result::Ok();

        if (errno != EINPROGRESS)
            return Result::Err(
                util::Error::from_errno(
                    errno, "connect failed"));

        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Result::Err(poller_res.unwrap_err());

        auto poller = std::move(poller_res.unwrap());
        poller.add(m_fd, EPOLLOUT).unwrap();

        auto deadline = time::deadline_after(timeout);

        while (true)
        {
            auto now = time::monotonic_now();
            if (time::expired(deadline))
                return Result::Err(
                    util::Error::internal(
                        "TCP connect timeout"));

            int ms = std::chrono::duration_cast<
                         std::chrono::milliseconds>(
                         deadline - now)
                         .count();

            auto ev = poller.wait(ms).unwrap();
            if (!ev.empty())
                break;
        }

        int so_error = 0;
        socklen_t len = sizeof(so_error);
        ::getsockopt(
            view().fd,
            SOL_SOCKET,
            SO_ERROR,
            &so_error,
            &len);

        if (so_error != 0)
            return Result::Err(
                util::Error::from_errno(
                    so_error, "connect failed"));

        return Result::Ok();
    }

    util::ResultV<size_t>
    TCPSocket::send_all(
        const std::byte *data,
        size_t len,
        time::Duration timeout)
    {
        using Result = util::ResultV<size_t>;

        NonBlockingGuard guard(view());

        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Result::Err(poller_res.unwrap_err());

        auto poller = std::move(poller_res.unwrap());
        poller.add(m_fd, EPOLLOUT).unwrap();

        size_t sent = 0;
        auto deadline = time::deadline_after(timeout);

        while (sent < len)
        {
            ssize_t n = ::send(
                view().fd,
                data + sent,
                len - sent,
                0);

            if (n > 0)
            {
                sent += static_cast<size_t>(n);
                continue;
            }

            if (errno != EAGAIN && errno != EWOULDBLOCK)
                return Result::Err(
                    util::Error::from_errno(
                        errno, "send failed"));

            auto now = time::monotonic_now();
            if (time::expired(deadline))
                return Result::Err(
                    util::Error::internal(
                        "TCP send timeout"));

            int ms =
                std::chrono::duration_cast<
                    std::chrono::milliseconds>(
                    deadline - now)
                    .count();
            poller.wait(ms).unwrap();
        }

        return Result::Ok(sent);
    }

    util::ResultV<size_t>
    TCPSocket::recv_some(
        std::byte *buf,
        size_t len,
        time::Duration timeout)
    {
        using Result = util::ResultV<size_t>;

        NonBlockingGuard guard(m_fd.view());

        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Result::Err(poller_res.unwrap_err());

        auto poller = std::move(poller_res.unwrap());
        poller.add(m_fd, EPOLLIN).unwrap();

        auto deadline = time::deadline_after(timeout);

        while (true)
        {
            ssize_t n = ::recv(view().fd, buf, len, 0);

            if (n >= 0)
                return util::ResultV<size_t>::Ok(
                    static_cast<size_t>(n));

            if (errno != EAGAIN && errno != EWOULDBLOCK)
                return util::ResultV<size_t>::Err(
                    util::Error::from_errno(
                        errno, "recv failed"));

            auto now = time::monotonic_now();
            if (time::expired(deadline))
                return Result::Err(
                    util::Error::internal(
                        "TCP recv timeout"));

            int ms =
                std::chrono::duration_cast<
                    std::chrono::milliseconds>(
                    deadline - now)
                    .count();
            poller.wait(ms).unwrap();
        }
    }
}
