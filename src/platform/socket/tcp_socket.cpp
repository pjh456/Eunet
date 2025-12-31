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
    TCPSocket::connect(const Endpoint &ep)
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

        if (err_no != EINPROGRESS)
        {
            // 立即失败
            return Result::Err(
                Error::transport()
                    .code(err_no)
                    .message("connect failed")
                    .build());
        }

        // EINPROGRESS: 连接进行中，等待可写
        auto poller_res = poller::Poller::create();
        if (poller_res.is_err())
        {
            return Result::Err(
                Error::system()
                    .message("poller create failed")
                    .wrap(poller_res.unwrap_err())
                    .build());
        }
        auto poller = std::move(poller_res.unwrap());

        auto add_res = poller.add(m_fd, EPOLLOUT);
        if (add_res.is_err())
        {
            return Result::Err(
                Error::system()
                    .message("epoll add fd error")
                    .wrap(add_res.unwrap_err())
                    .build());
        }

        auto events_res = poller.wait(-1);
        if (events_res.is_err())
        {
            return Result::Err(
                Error::system()
                    .message("get poller event failed")
                    .wrap(events_res.unwrap_err())
                    .build());
        }

        auto events = events_res.unwrap();
        bool writable = false;
        for (auto &ev : events)
        {
            if (ev.fd == view() && ev.is_writable())
            {
                writable = true;
                break;
            }
        }

        // 如果没有可写事件，说明连接没有成功或超时
        if (!writable)
        {
            return Result::Err(
                Error::transport()
                    .timeout()
                    .message("connect did not complete (timeout or error)")
                    .build());
        }

        // 检查连接是否真正成功
        int sock_err = 0;
        socklen_t len = sizeof(sock_err);
        if (::getsockopt(view().fd, SOL_SOCKET, SO_ERROR, &sock_err, &len) < 0)
        {
            int e = errno;
            return Result::Err(
                Error::system()
                    .code(e)
                    .message("getsockopt failed after connect")
                    .build());
        }

        if (sock_err != 0)
        {
            return Result::Err(
                Error::transport()
                    .code(sock_err)
                    .message("connect failed after poll")
                    .build());
        }

        return Result::Ok();
    }

    IOResult
    TCPSocket::read(
        util::ByteBuffer &buf,
        time::Duration timeout)
    {
        using util::Error;
        using namespace platform::poller;

        NonBlockingGuard guard(view());
        size_t total_read = 0;

        // 创建 poller
        auto poller_res = Poller::create();
        if (poller_res.is_err())
        {
            return IOResult::Err(
                Error::system()
                    .message("poller create failed")
                    .wrap(poller_res.unwrap_err())
                    .build());
        }

        auto poller = std::move(poller_res.unwrap());

        auto add_res = poller.add(m_fd, EPOLLIN);
        if (add_res.is_err())
        {
            return IOResult::Err(
                Error::system()
                    .message("epoll add fd failed")
                    .wrap(add_res.unwrap_err())
                    .build());
        }

        auto start = platform::time::monotonic_now();
        while (buf.writable_size() > 0)
        {
            auto r = try_read(buf);
            if (r.is_err())
            {
                auto err = r.unwrap_err();
                if (err.domain() == util::ErrorDomain::Transport &&
                    err.code() == 0) // peer closed
                    return IOResult::Err(err);
            }

            size_t n = r.unwrap();
            total_read += n;
            if (n > 0)
                continue;

            // 等待可读事件
            auto elapsed_ms =
                time::elapsed(
                    start, platform::time::monotonic_now())
                    .count();

            int wait_ms = static_cast<int>(timeout.count()) - elapsed_ms;
            if (wait_ms <= 0)
                return IOResult::Err(
                    util::Error::transport()
                        .timeout()
                        .message("read timeout")
                        .build());

            auto events_res = poller.wait(wait_ms);
            if (events_res.is_err())
                return IOResult::Err(events_res.unwrap_err());

            bool readable = false;
            for (auto &ev : events_res.unwrap())
            {
                if (ev.fd == view() && (ev.events & EPOLLIN))
                {
                    readable = true;
                    break;
                }
            }
            if (!readable)
                return IOResult::Err(
                    util::Error::transport()
                        .timeout()
                        .message("read timeout or not readable")
                        .build());
        }

        return IOResult::Ok(total_read);
    }

    IOResult
    TCPSocket::write(
        util::ByteBuffer &buf,
        platform::time::Duration timeout)
    {
        using Ret = util::ResultV<size_t>;
        using util::Error;
        using namespace platform::poller;

        NonBlockingGuard guard(view());
        size_t total_written = 0;

        auto poller_res = Poller::create();
        if (poller_res.is_err())
            return Ret::Err(poller_res.unwrap_err());

        auto poller = std::move(poller_res.unwrap());

        auto add_res = poller.add(m_fd, EPOLLIN);
        if (add_res.is_err())
        {
            return IOResult::Err(
                Error::system()
                    .message("epoll add fd failed")
                    .wrap(add_res.unwrap_err())
                    .build());
        }

        auto start = platform::time::monotonic_now();
        while (buf.size() > 0)
        {
            auto r = try_write(buf);
            if (r.is_err())
            {
                auto err = r.unwrap_err();
                if (err.domain() == util::ErrorDomain::Transport &&
                    err.code() == 0) // peer closed
                    return Ret::Err(err);
            }

            size_t n = r.unwrap();
            total_written += n;
            if (n > 0)
                continue;

            // 等待可写事件
            auto elapsed_ms =
                time::elapsed(
                    start, platform::time::monotonic_now())
                    .count();

            int wait_ms = static_cast<int>(timeout.count()) - elapsed_ms;
            if (wait_ms <= 0)
                return Ret::Err(
                    util::Error::transport()
                        .timeout()
                        .message("write timeout")
                        .build());

            auto events_res = poller.wait(wait_ms);
            if (events_res.is_err())
                return Ret::Err(events_res.unwrap_err());

            bool writable = false;
            for (auto &ev : events_res.unwrap())
            {
                if (ev.fd == view() && (ev.events & EPOLLOUT))
                {
                    writable = true;
                    break;
                }
            }
            if (!writable)
                return Ret::Err(
                    util::Error::transport()
                        .timeout()
                        .message("write timeout or not writable")
                        .build());
        }

        return Ret::Ok(total_written);
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
