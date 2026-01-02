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

    IOResult
    TCPSocket::read(
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
                view().fd,
                span.data(),
                writable,
                0);

            if (n > 0)
            {
                buf.weak_commit(n);
                return Ret::Ok(static_cast<size_t>(n));
            }

            if (n == 0)
            {
                return Ret::Err(
                    Error::transport()
                        .peer_closed()
                        .message("Connection closed by peer")
                        .context("recv")
                        .build());
            }

            int err = errno;
            if (err == EINTR)
                continue;

            if (err == EAGAIN || err == EWOULDBLOCK)
            {
                auto w = wait_fd_epoll(
                    poller, view(),
                    EPOLLIN, timeout_ms);

                if (w.is_err())
                    return Ret::Err(w.unwrap_err());

                continue;
            }

            return Ret::Err(
                Error::transport()
                    .code(err)
                    .set_category(from_errno(err))
                    .message("Failed to receive data from TCP socket")
                    .context("read")
                    .build());
        }
    }

    IOResult
    TCPSocket::write(
        util::ByteBuffer &buf,
        int timeout_ms)
    {
        using Ret = IOResult;
        using util::Error;

        poller::Poller poller =
            poller::Poller::create().unwrap();

        while (!buf.empty())
        {
            auto data = buf.readable();

            ssize_t n = ::send(
                view().fd,
                data.data(),
                data.size(),
                0);

            if (n > 0)
            {
                buf.consume(n);
                return Ret::Ok(static_cast<size_t>(n));
            }

            if (n == 0)
            {
                return Ret::Err(
                    Error::transport()
                        .peer_closed()
                        .message("Connection closed by peer")
                        .context("recv")
                        .build());
            }

            int err = errno;
            if (err == EINTR)
                continue;

            if (err == EAGAIN || err == EWOULDBLOCK)
            {
                auto w = wait_fd_epoll(
                    poller, view(),
                    EPOLLOUT, timeout_ms);

                if (w.is_err())
                    return Ret::Err(w.unwrap_err());

                continue;
            }

            return Ret::Err(
                Error::transport()
                    .code(err)
                    .set_category(from_errno(err))
                    .message("Failed to send data to TCP socket")
                    .context("write")
                    .build());
        }

        return Ret::Ok(0);
    }

    util::ResultV<void>
    TCPSocket::connect(
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
                    .set_category(from_errno(errno))
                    .message("Immediate TCP connection attempt failed")
                    .context("connect")
                    .build());
        }

        poller::Poller poller =
            poller::Poller::create().unwrap();

        auto w = wait_fd_epoll(
            poller, view(),
            EPOLLOUT, timeout_ms);

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
                    .set_category(from_errno(err))
                    .message("Async TCP connection attempt failed")
                    .context("getsockopt(SO_ERROR)")
                    .build());
        }

        return Result::Ok();
    }

}
