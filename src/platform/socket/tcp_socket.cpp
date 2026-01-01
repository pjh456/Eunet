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

    util::ResultV<size_t>
    TCPSocket::try_read(
        util::ByteBuffer &buf)
    {
        using Ret = util::ResultV<size_t>;
        using util::Error;

        NonBlockingGuard guard(view());

        auto buf_len = buf.writable_size();
        auto buffer = buf.weak_prepare(buf_len);
        ssize_t n = ::recv(view().fd, buffer.data(), buf_len, 0);

        if (n > 0)
        {
            buf.weak_commit(n);
            return Ret::Ok(static_cast<size_t>(n));
        }
        else if (n == 0)
        {
            return Ret::Err(
                Error::transport()
                    .peer_closed()
                    .code(0)
                    .message("peer closed")
                    .build());
        }

        int err_no = errno;
        if (err_no == EAGAIN || err_no == EWOULDBLOCK)
            return Ret::Ok(0);

        return Ret::Err(
            Error::transport()
                .code(err_no)
                .message("recv failed")
                .build());
    }

    util::ResultV<size_t>
    TCPSocket::try_write(
        util::ByteBuffer &buf)
    {
        using Ret = util::ResultV<size_t>;
        using util::Error;

        NonBlockingGuard guard(view());

        auto data_len = buf.size();
        auto data = buf.readable();
        ssize_t n = ::send(view().fd, data.data(), data_len, 0);

        if (n > 0)
        {
            buf.consume(n);
            return Ret::Ok(static_cast<size_t>(n));
        }
        else if (n == 0)
        {
            return Ret::Err(
                Error::transport()
                    .peer_closed()
                    .code(0)
                    .message("peer closed")
                    .build());
        }

        int err_no = errno;
        if (err_no == EAGAIN || err_no == EWOULDBLOCK)
            return Ret::Ok(0);

        return Ret::Err(
            Error::transport()
                .code(err_no)
                .message("send failed")
                .build());
    }

    util::ResultV<void>
    TCPSocket::try_connect(
        const Endpoint &ep)
    {
        using Result = util::ResultV<void>;
        using util::Error;

        // 非阻塞连接
        NonBlockingGuard guard(view());

        int ret = ::connect(
            view().fd,
            ep.as_sockaddr(),
            ep.length());

        if (ret == 0)
            return Result::Ok(); // 立即连接成功

        int err_no = errno;
        if (err_no == EINPROGRESS)
            return Result::Ok();

        return Result::Err(
            Error::transport()
                .code(err_no)
                .message("connect failed")
                .build());
    }

    util::ResultV<Endpoint>
    TCPSocket::local_endpoint() const
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
    TCPSocket::remote_endpoint() const
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
}
