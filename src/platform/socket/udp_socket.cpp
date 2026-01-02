#include "eunet/platform/socket/udp_socket.hpp"
#include "eunet/platform/poller.hpp"

#include <sys/socket.h>
#include <cerrno>

namespace platform::net
{

    util::ResultV<UDPSocket>
    UDPSocket::create(
        AddressFamily af)
    {
        using Result = util::ResultV<UDPSocket>;

        int domain =
            (af == AddressFamily::IPv6)
                ? AF_INET6
                : AF_INET;

        auto fd_res =
            fd::Fd::socket(
                domain,
                SOCK_DGRAM,
                0);

        if (fd_res.is_err())
            return Result::Err(fd_res.unwrap_err());

        return Result::Ok(
            UDPSocket(std::move(fd_res.unwrap())));
    }

    IOResult
    UDPSocket::read(
        util::ByteBuffer &buf,
        int timeout_ms)
    {
        using Ret = IOResult;
        using util::Error;

        poller::Poller poller =
            poller::Poller::create().unwrap();

        for (;;)
        {
            auto writable = buf.writable_size();
            auto span = buf.weak_prepare(writable);

            ssize_t n = ::recv(
                view().fd, span.data(),
                writable, MSG_DONTWAIT);

            if (n >= 0)
            {
                buf.weak_commit(static_cast<size_t>(n));
                return Ret::Ok(static_cast<size_t>(n));
            }

            int err = errno;
            if (err == EINTR)
                continue;

            if (err == EAGAIN || err == EWOULDBLOCK)
            {
                if (timeout_ms == 0)
                    return Ret::Ok(0);

                auto w = wait_fd_epoll(
                    poller,
                    view(),
                    EPOLLIN,
                    timeout_ms);

                if (w.is_err())
                    return Ret::Err(w.unwrap_err());

                continue;
            }

            return Ret::Err(
                Error::transport()
                    .code(err)
                    .message("udp recv failed")
                    .build());
        }
    }

    IOResult
    UDPSocket::write(
        util::ByteBuffer &buf,
        int timeout_ms)
    {
        using Ret = IOResult;
        using util::Error;

        poller::Poller poller =
            poller::Poller::create().unwrap();

        if (buf.empty())
            return Ret::Ok(0);

        for (;;)
        {
            auto data = buf.readable();

            ssize_t n = ::send(
                view().fd,
                data.data(),
                data.size(),
                0);

            if (n >= 0)
            {
                buf.consume(static_cast<size_t>(n));
                return Ret::Ok(static_cast<size_t>(n));
            }

            int err = errno;
            if (err == EINTR)
                continue;

            if (err == EAGAIN || err == EWOULDBLOCK)
            {
                auto w = wait_fd_epoll(
                    poller,
                    view(),
                    EPOLLOUT,
                    timeout_ms);

                if (w.is_err())
                    return Ret::Err(w.unwrap_err());

                continue;
            }

            return Ret::Err(
                Error::transport()
                    .code(err)
                    .message("udp send failed")
                    .build());
        }
    }

    util::ResultV<void>
    UDPSocket::connect(
        const Endpoint &ep,
        int timeout_ms)
    {
        using Result = util::ResultV<void>;
        using util::Error;

        int ret = ::connect(
            view().fd,
            ep.as_sockaddr(),
            ep.length());

        if (ret == 0)
            return Result::Ok();

        if (errno != EINPROGRESS)
        {
            return Result::Err(
                Error::transport()
                    .code(errno)
                    .message("udp connect failed")
                    .build());
        }

        poller::Poller poller =
            poller::Poller::create().unwrap();

        auto w = wait_fd_epoll(
            poller,
            view(),
            EPOLLOUT,
            timeout_ms);

        if (w.is_err())
            return Result::Err(w.unwrap_err());

        int err = 0;
        socklen_t len = sizeof(err);
        ::getsockopt(
            view().fd,
            SOL_SOCKET,
            SO_ERROR,
            &err,
            &len);

        if (err != 0)
        {
            return Result::Err(
                Error::transport()
                    .code(err)
                    .message("udp connect failed")
                    .build());
        }

        return Result::Ok();
    }
}