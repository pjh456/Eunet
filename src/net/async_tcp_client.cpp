#include "eunet/net/async_tcp_client.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <errno.h>

namespace net::tcp
{
    AsyncTCPClient::AsyncTCPClient(core::Orchestrator &o) : orch(o) {}

    util::ResultV<void> AsyncTCPClient::emit_event(const core::Event &e) { return orch.emit(e); }

    util::ResultV<void>
    AsyncTCPClient::set_nonblocking(int fd)
    {
        using Ret = util::ResultV<void>;
        using util::Error;

        int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags == -1)
        {
            int err_no = errno;
            return Ret::Err(
                Error::system()
                    .invalid_input()
                    .code(err_no)
                    .message("Failed to get socket flags")
                    .context("fcntl(F_GETFL)")
                    .build());
        }

        if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            int err_no = errno;
            return Ret::Err(
                Error::system()
                    .resource_exhausted()
                    .code(err_no)
                    .message("Failed to set socket non-blocking")
                    .context("fcntl(F_SETFL)")
                    .build());
        }

        return Ret::Ok();
    }

    util::ResultV<sockaddr_in>
    AsyncTCPClient::resolve_host(const std::string &host, uint16_t port)
    {
        using Ret = util::ResultV<sockaddr_in>;
        using util::Error;

        (void)emit_event(core::Event::info(core::EventType::DNS_RESOLVE_START, "Resolving: " + host));

        addrinfo hints{}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int s = getaddrinfo(host.c_str(), nullptr, &hints, &res);
        if (s != 0)
        {
            auto err =
                Error::dns()
                    .unreachable()
                    .code(s)
                    .message("DNS resolution failed")
                    .context(host)
                    .build();
            (void)emit_event(core::Event::failure(core::EventType::DNS_RESOLVE_DONE, err));
            return Ret::Err(err);
        }

        sockaddr_in addr = *(reinterpret_cast<sockaddr_in *>(res->ai_addr));
        addr.sin_port = htons(port);
        freeaddrinfo(res);

        (void)emit_event(core::Event::info(core::EventType::DNS_RESOLVE_DONE, "Resolved: " + host));
        return Ret::Ok(addr);
    }

    util::ResultV<void>
    AsyncTCPClient::connect(
        const std::string &host,
        uint16_t port,
        int timeout_ms)
    {
        using Ret = util::ResultV<void>;
        using util::Error;

        // 1. DNS 解析
        auto addr_res = resolve_host(host, port);
        if (addr_res.is_err())
            return Ret::Err(addr_res.unwrap_err());
        auto addr = addr_res.unwrap();

        // 2. 创建 Socket
        auto fd_res = platform::fd::Fd::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_res.is_err())
            return Ret::Err(fd_res.unwrap_err());
        sock = std::move(fd_res.unwrap());

        // 3. 设置非阻塞
        auto nb_res = set_nonblocking(sock.get());
        if (nb_res.is_err())
            return Ret::Err(nb_res.unwrap_err());

        // 4. 发起连接
        (void)emit_event(core::Event::info(core::EventType::TCP_CONNECT_START, "Connecting (non-blocking)...", sock.view()));

        int ret = ::connect(sock.get(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));

        if (ret < 0 && errno != EINPROGRESS)
        {
            int err_no = errno;
            auto err =
                util::Error::transport()
                    .set_category(connect_errno_category(err_no))
                    .code(err_no)
                    .message("TCP connect failed")
                    .context(host)
                    .build();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::TCP_CONNECT_START,
                    err,
                    sock.view()));

            return Ret::Err(err);
        }

        if (ret == 0)
        {
            // 极少数情况：连接立即完成了
            (void)emit_event(core::Event::info(core::EventType::TCP_CONNECT_SUCCESS, "Connected immediately", sock.view()));
            return Ret::Ok();
        }

        // 5. 使用 Poller 等待连接完成 (等待可写)
        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return Ret::Err(poller_res.unwrap_err());
        auto poller = std::move(poller_res.unwrap());

        // 监控可写(EPOLLOUT)和错误(EPOLLERR)
        (void)poller.add(sock.view(), EPOLLOUT | EPOLLERR | EPOLLHUP);
        auto wait_res = poller.wait(timeout_ms);

        if (wait_res.is_err())
            return Ret::Err(wait_res.unwrap_err());
        auto events = wait_res.unwrap();

        if (events.empty())
        {
            auto err =
                Error::transport()
                    .timeout()
                    .message("TCP connection timeout")
                    .context(host)
                    .build();
            (void)emit_event(core::Event::failure(core::EventType::TCP_CONNECT_TIMEOUT, err, sock.view()));
            return Ret::Err(err);
        }

        // 6. 检查连接最终结果
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        if (getsockopt(sock.get(), SOL_SOCKET, SO_ERROR, &so_error, &len) < 0)
        {
            int err_no = errno;
            return Ret::Err(
                Error::system()
                    .code(err_no)
                    .message("getsockopt SO_ERROR")
                    .build());
        }

        if (so_error != 0)
        {
            auto err =
                Error::transport()
                    .set_category(connect_errno_category(so_error))
                    .code(so_error)
                    .message("TCP connection failed")
                    .context(host)
                    .build();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::TCP_CONNECT_START,
                    err,
                    sock.view()));

            return Ret::Err(err);
        }

        (void)emit_event(core::Event::info(core::EventType::TCP_CONNECT_SUCCESS, "Connected established", sock.view()));
        return Ret::Ok();
    }

    util::ResultV<size_t>
    AsyncTCPClient::send(
        const std::vector<std::byte> &data)
    {
        using Ret = util::ResultV<size_t>;
        using util::Error;
        if (!sock)
        {
            return Ret::Err(
                Error::create()
                    .system()
                    .message("Socket not connected")
                    .build());
        }

        (void)emit_event(core::Event::info(core::EventType::HTTP_SENT, "Sending data...", sock.view()));

        ssize_t n = ::send(sock.get(), data.data(), data.size(), 0);
        if (n < 0)
        {
            int err_no = errno;
            auto err =
                Error::transport()
                    .set_category(
                        err_no == EPIPE ? util::ErrorCategory::ConnectionRefused
                                        : util::ErrorCategory::Unknown)
                    .code(err_no)
                    .message("Send failed")
                    .build();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::HTTP_SENT,
                    err,
                    sock.view()));

            return Ret::Err(err);
        }
        return Ret::Ok(static_cast<size_t>(n));
    }

    util::ResultV<size_t>
    AsyncTCPClient::recv(
        std::vector<std::byte> &buffer,
        size_t max_size)
    {
        using Ret = util::ResultV<size_t>;
        using util::Error;
        // 简化版实现：实际生产中可能需要 Poller 等待可读
        buffer.resize(max_size);
        ssize_t n = ::recv(sock.get(), buffer.data(), max_size, 0);

        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return util::ResultV<size_t>::Ok(0);

            int err_no = errno;
            auto err =
                Error::transport()
                    .protocol_error()
                    .code(errno)
                    .message("Receive failed")
                    .build();
            return Ret::Err(err);
        }

        buffer.resize(n);
        if (n > 0)
            (void)emit_event(core::Event::info(core::EventType::HTTP_RECEIVED, "Data received", sock.view()));
        return util::ResultV<size_t>::Ok(static_cast<size_t>(n));
    }

    void AsyncTCPClient::close() noexcept
    {
        if (sock)
        {
            (void)emit_event(core::Event::info(core::EventType::CONNECTION_CLOSED, "Closing", sock.view()));
            sock.reset(-1);
        }
    }

    util::ErrorCategory
    AsyncTCPClient::connect_errno_category(int err)
    {
        using util::ErrorCategory;
        switch (err)
        {
        case ECONNREFUSED:
            return ErrorCategory::ConnectionRefused;
        case ETIMEDOUT:
            return ErrorCategory::Timeout;
        case ENETUNREACH:
        case EHOSTUNREACH:
            return ErrorCategory::Unreachable;
        case EACCES:
            return ErrorCategory::AccessDenied;
        default:
            return ErrorCategory::Unknown;
        }
    }
}