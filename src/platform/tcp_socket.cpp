#include "eunet/platform/tcp_socket.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <poll.h>

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
                    errno, "Failed to create socket"));
        return Result::Ok(TCPSocket(fd::Fd(sockfd)));
    }

    TCPSocket::TCPSocket(fd::Fd fd)
        : fd(std::move(fd)) {}

    util::ResultV<void>
    TCPSocket::connect(
        const SocketAddress &addr,
        time::Duration timeout)
    {
        using Result = util::ResultV<void>;

        set_nonblocking(true);

        int ret = ::connect(
            fd.view().fd,
            addr.as_sockaddr(),
            addr.length());
        if (ret == 0)
        {
            // 立即连接成功
            set_nonblocking(false);
            return Result::Ok();
        }
        else if (errno != EINPROGRESS)
            return Result::Err(
                util::Error::from_errno(
                    errno, "Connect failed"));

        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Result::Err(
                util::Error::internal(
                    "Failed to create poller: " +
                    poller_res.unwrap_err().message()));

        auto poller = std::move(poller_res.unwrap());
        auto add_res = poller.add(fd, EPOLLOUT);
        if (add_res.is_err())
            return Result::Err(
                util::Error::internal(
                    "Failed to add fd to poller: " +
                    add_res.unwrap_err().message()));

        int timeout_ms =
            static_cast<int>(
                std::chrono::duration_cast<
                    std::chrono::milliseconds>(timeout)
                    .count());

        auto events_res = poller.wait(timeout_ms);
        if (events_res.is_err())
            return Result::Err(
                util::Error::internal(
                    "Poller wait failed: " +
                    events_res.unwrap_err().message()));

        auto events = events_res.unwrap();
        if (events.empty())
            return Result::Err(
                util::Error::internal(
                    "Connect timeout"));

        // 检查 SO_ERROR
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        if (getsockopt(
                fd.view().fd,
                SOL_SOCKET, SO_ERROR,
                &so_error, &len) < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "getsockopt failed"));
        if (so_error != 0)
            return Result::Err(
                util::Error::from_errno(
                    so_error, "Connect failed"));

        set_nonblocking(false);
        return Result::Ok();
    }

    util::ResultV<size_t>
    TCPSocket::send(
        const std::byte *data,
        size_t len, time::Duration timeout)
    {
        using Result = util::ResultV<size_t>;

        set_nonblocking(true);

        size_t total_sent = 0;
        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Result::Err(
                util::Error::internal(
                    "Failed to create poller: " +
                    poller_res.unwrap_err().message()));

        auto poller = std::move(poller_res.unwrap());

        while (total_sent < len)
        {
            auto add_res = poller.add(fd, EPOLLOUT);
            if (add_res.is_err())
                return Result::Err(
                    util::Error::internal(
                        "Failed to add fd to poller: " +
                        add_res.unwrap_err().message()));

            int timeout_ms = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
            auto events_res = poller.wait(timeout_ms);
            if (events_res.is_err())
                return Result::Err(
                    util::Error::internal(
                        "Poller wait failed: " +
                        events_res.unwrap_err().message()));

            auto events = events_res.unwrap();
            if (events.empty())
                return Result::Err(
                    util::Error::internal(
                        "Send timeout"));

            ssize_t n = ::send(
                fd.view().fd,
                data + total_sent,
                len - total_sent, 0);
            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue; // 再次等待
                return Result::Err(
                    util::Error::from_errno(
                        errno, "Send failed"));
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

        size_t total_received = 0;
        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Result::Err(
                util::Error::internal(
                    "Failed to create poller: " +
                    poller_res.unwrap_err().message()));

        auto poller = std::move(poller_res.unwrap());

        while (total_received < len)
        {
            auto add_res = poller.add(fd, EPOLLIN);
            if (add_res.is_err())
                return Result::Err(
                    util::Error::internal(
                        "Failed to add fd to poller: " +
                        add_res.unwrap_err().message()));

            int timeout_ms = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
            auto events_res = poller.wait(timeout_ms);
            if (events_res.is_err())
                return Result::Err(
                    util::Error::internal(
                        "Poller wait failed: " +
                        events_res.unwrap_err().message()));

            auto events = events_res.unwrap();
            if (events.empty())
                return Result::Err(
                    util::Error::internal(
                        "Recv timeout"));

            ssize_t n = ::recv(
                fd.view().fd,
                buf + total_received,
                len - total_received, 0);
            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue; // 再次等待
                return Result::Err(
                    util::Error::from_errno(
                        errno,
                        "Recv failed"));
            }
            else if (n == 0)
                break; // 对端关闭

            total_received += static_cast<size_t>(n);
        }

        set_nonblocking(false);
        return Result::Ok(total_received);
    }

    void TCPSocket::set_nonblocking(bool enable) noexcept
    {
        int flags = fcntl(fd.view().fd, F_GETFL, 0);
        if (flags < 0)
            return;
        if (enable)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;
        fcntl(fd.view().fd, F_SETFL, flags);
    }

    fd::FdView TCPSocket::view() const noexcept { return fd.view(); }

    void TCPSocket::close() noexcept { fd.reset(-1); }
}