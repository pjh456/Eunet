#include "eunet/platform/socket/tcp_socket.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>

#include "eunet/platform/poller.hpp"

namespace platform::net
{
    util::ResultV<TCPSocket>
    TCPSocket::create()
    {
        using Result = util::ResultV<TCPSocket>;

        int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "Failed to create TCP socket"));

        return Result::Ok(TCPSocket(fd::Fd(sockfd)));
    }

    TCPSocket::TCPSocket(fd::Fd fd)
        : SocketBase(std::move(fd)) {}

    util::ResultV<void>
    TCPSocket::connect(
        const SocketAddress &addr,
        time::Duration timeout)
    {
        using Result = util::ResultV<void>;

        set_nonblocking(true);

        int ret = ::connect(
            m_fd.view().fd,
            addr.as_sockaddr(),
            addr.length());

        if (ret == 0)
        {
            set_nonblocking(false);
            return Result::Ok();
        }

        if (errno != EINPROGRESS)
            return Result::Err(
                util::Error::from_errno(
                    errno, "TCP connect failed"));

        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Result::Err(
                util::Error::internal(
                    "Failed to create poller: " +
                    poller_res.unwrap_err().message()));

        auto poller = std::move(poller_res.unwrap());
        if (auto res = poller.add(m_fd, EPOLLOUT); res.is_err())
            return Result::Err(res.unwrap_err());

        int timeout_ms =
            static_cast<int>(
                std::chrono::duration_cast<
                    std::chrono::milliseconds>(timeout)
                    .count());

        auto events_res = poller.wait(timeout_ms);
        if (events_res.is_err())
            return Result::Err(events_res.unwrap_err());

        if (events_res.unwrap().empty())
            return Result::Err(
                util::Error::internal("TCP connect timeout"));

        int so_error = 0;
        socklen_t len = sizeof(so_error);
        if (::getsockopt(
                m_fd.view().fd,
                SOL_SOCKET,
                SO_ERROR,
                &so_error,
                &len) < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "getsockopt(SO_ERROR) failed"));

        if (so_error != 0)
            return Result::Err(
                util::Error::from_errno(
                    so_error, "TCP connect failed"));

        set_nonblocking(false);
        return Result::Ok();
    }

    util::ResultV<size_t>
    TCPSocket::send(
        const std::byte *data,
        size_t len,
        time::Duration timeout)
    {
        using Result = util::ResultV<size_t>;

        set_nonblocking(true);

        size_t total_sent = 0;
        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Result::Err(poller_res.unwrap_err());

        auto poller = std::move(poller_res.unwrap());

        while (total_sent < len)
        {
            if (auto res = poller.add(m_fd, EPOLLOUT); res.is_err())
                return Result::Err(res.unwrap_err());

            int timeout_ms =
                static_cast<int>(
                    std::chrono::duration_cast<
                        std::chrono::milliseconds>(timeout)
                        .count());

            auto events_res = poller.wait(timeout_ms);
            if (events_res.is_err())
                return Result::Err(events_res.unwrap_err());

            if (events_res.unwrap().empty())
                return Result::Err(
                    util::Error::internal("TCP send timeout"));

            ssize_t n = ::send(
                m_fd.view().fd,
                data + total_sent,
                len - total_sent,
                0);

            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;

                return Result::Err(
                    util::Error::from_errno(
                        errno, "TCP send failed"));
            }

            total_sent += static_cast<size_t>(n);
        }

        set_nonblocking(false);
        return Result::Ok(total_sent);
    }

    util::ResultV<size_t>
    TCPSocket::recv(
        std::byte *buf,
        size_t len,
        time::Duration timeout)
    {
        using Result = util::ResultV<size_t>;

        set_nonblocking(true);

        size_t total_recv = 0;
        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Result::Err(poller_res.unwrap_err());

        auto poller = std::move(poller_res.unwrap());

        while (total_recv < len)
        {
            if (auto res = poller.add(m_fd, EPOLLIN); res.is_err())
                return Result::Err(res.unwrap_err());

            int timeout_ms =
                static_cast<int>(
                    std::chrono::duration_cast<
                        std::chrono::milliseconds>(timeout)
                        .count());

            auto events_res = poller.wait(timeout_ms);
            if (events_res.is_err())
                return Result::Err(events_res.unwrap_err());

            if (events_res.unwrap().empty())
                return Result::Err(
                    util::Error::internal("TCP recv timeout"));

            ssize_t n = ::recv(
                m_fd.view().fd,
                buf + total_recv,
                len - total_recv,
                0);

            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;

                return Result::Err(
                    util::Error::from_errno(
                        errno, "TCP recv failed"));
            }

            if (n == 0)
                break; // peer closed

            total_recv += static_cast<size_t>(n);
        }

        set_nonblocking(false);
        return Result::Ok(total_recv);
    }
}