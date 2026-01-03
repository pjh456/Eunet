#include "eunet/platform/socket/tcp_socket.hpp"
#include "eunet/platform/poller.hpp"
#include "eunet/platform/time.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <cerrno>

namespace platform::net
{

    util::ResultV<TCPSocket>
    TCPSocket::create(
        poller::Poller &poller,
        AddressFamily af)
    {
        using Result = util::ResultV<TCPSocket>;

        int domain = (af == AddressFamily::IPv6)
                         ? AF_INET6
                         : AF_INET;

        auto fd_res =
            fd::Fd::socket(
                domain,
                SOCK_STREAM,
                0);

        if (fd_res.is_err())
            return Result::Err(fd_res.unwrap_err());

        return Result::Ok(
            TCPSocket(std::move(fd_res.unwrap()), poller));
    }

    IOResult
    TCPSocket::read(
        util::ByteBuffer &buf,
        int timeout_ms)
    {
        using Ret = IOResult;
        using util::Error;

        // 进入读取循环以处理可能的信号中断或重试逻辑
        for (;;)
        {
            // 获取缓冲区当前可写入的空间大小
            // 获取该空间的弱引用视图 此时不更新 pending 状态
            auto writable = buf.writable_size();
            auto span = buf.weak_prepare(writable);

            // 调用系统调用 recv 尝试读取数据
            ssize_t n = ::recv(
                view().fd,
                span.data(),
                writable,
                0);

            // 如果返回值大于 0 表示成功读取到了数据
            if (n > 0)
            {
                // 提交写入 更新缓冲区的 write_pos
                buf.weak_commit(n);
                return Ret::Ok(static_cast<size_t>(n));
            }

            // 如果返回值为 0 表示对端已经正常关闭了连接 FIN
            if (n == 0)
            {
                return Ret::Err(
                    Error::create()
                        .success()
                        .peer_closed()
                        .message("Connection closed by peer")
                        .context("TCPSocket::read")
                        .build());
            }

            // 获取 errno 检查错误类型
            int err = errno;

            // 如果是被信号中断 则重新尝试循环
            if (err == EINTR)
                continue;

            // 如果是资源暂时不可用 EAGAIN 或 EWOULDBLOCK
            if (err == EAGAIN || err == EWOULDBLOCK)
            {
                // 调用 epoll 等待该 fd 变为可读
                auto w = wait_fd_epoll(
                    m_poller, view(),
                    EPOLLIN, timeout_ms);

                // 如果等待失败或超时 直接返回错误
                if (w.is_err())
                    return Ret::Err(w.unwrap_err());

                // 等待成功后 再次进入循环尝试 recv
                continue;
            }

            // 其他情况视为致命的系统错误
            return Ret::Err(
                Error::transport()
                    .code(err)
                    .set_category(from_errno(err))
                    .message("Failed to receive data from TCP socket")
                    .context("TCPSocket::read")
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

        // 进入读取循环以处理可能的信号中断或重试逻辑
        while (!buf.empty())
        {
            // 获取缓冲区当前可读取的空间大小
            auto data = buf.readable();

            // 调用系统调用 send 尝试写入数据
            ssize_t n = ::send(
                view().fd,
                data.data(),
                data.size(),
                0);

            // 如果返回值大于 0 表示成功写入了数据
            if (n > 0)
            {
                // 提交写入 更新缓冲区的 read_pos
                buf.consume(n);
                return Ret::Ok(static_cast<size_t>(n));
            }

            // 如果返回值为 0 表示对端已经正常关闭了连接 FIN
            if (n == 0)
            {
                return Ret::Err(
                    Error::transport()
                        .peer_closed()
                        .message("Connection closed by peer")
                        .context("TCPSocket::write")
                        .build());
            }

            // 获取 errno 检查错误类型
            int err = errno;

            // 如果是被信号中断 则重新尝试循环
            if (err == EINTR)
                continue;

            // 如果是资源暂时不可用 EAGAIN 或 EWOULDBLOCK
            if (err == EAGAIN || err == EWOULDBLOCK)
            {
                // 调用 epoll 等待该 fd 变为可读
                auto w = wait_fd_epoll(
                    m_poller, view(),
                    EPOLLOUT, timeout_ms);

                // 如果等待失败或超时 直接返回错误
                if (w.is_err())
                    return Ret::Err(w.unwrap_err());

                // 等待成功后 再次进入循环尝试 send
                continue;
            }

            // 其他情况视为致命的系统错误
            return Ret::Err(
                Error::transport()
                    .code(err)
                    .set_category(from_errno(err))
                    .message("Failed to send data to TCP socket")
                    .context("TCPSocket::write")
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

        // 调用非阻塞 connect 系统调用
        int ret = ::connect(
            view().fd,
            ep.as_sockaddr(),
            ep.length());

        // 如果返回 0 表示连接立即建立成功（主要见于本地环回）
        if (ret == 0)
            return Result::Ok();

        // 检查错误码 如果不是 EINPROGRESS 则表示连接立即失败
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

        // 连接正在进行中 使用 epoll 等待 socket 变为可写
        auto w = wait_fd_epoll(
            m_poller, view(),
            EPOLLOUT, timeout_ms);
        if (w.is_err())
            return Result::Err(w.unwrap_err());

        // epoll 返回可写并不一定代表成功 需要检查 socket 错误状态
        int err = 0;
        socklen_t len = sizeof(err);
        ::getsockopt(
            view().fd,
            SOL_SOCKET,
            SO_ERROR,
            &err,
            &len);

        // 如果 SO_ERROR 不为 0 说明异步连接过程中发生了错误
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

        // 没有任何错误 连接确认建立
        return Result::Ok();
    }

}
