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
        int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags == -1)
            return util::ResultV<void>::Err(util::Error::from_errno(errno, "fcntl F_GETFL"));
        if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
            return util::ResultV<void>::Err(util::Error::from_errno(errno, "fcntl F_SETFL O_NONBLOCK"));
        return util::ResultV<void>::Ok();
    }

    util::ResultV<sockaddr_in>
    AsyncTCPClient::resolve_host(const std::string &host, uint16_t port)
    {
        (void)emit_event(core::Event::info(core::EventType::DNS_RESOLVE_START, "Resolving: " + host));

        addrinfo hints{}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int s = getaddrinfo(host.c_str(), nullptr, &hints, &res);
        if (s != 0)
        {
            auto err = util::Error::from_gai(s, "DNS lookup failed");
            (void)emit_event(core::Event::failure(core::EventType::DNS_RESOLVE_DONE, err));
            return util::ResultV<sockaddr_in>::Err(err);
        }

        sockaddr_in addr = *(reinterpret_cast<sockaddr_in *>(res->ai_addr));
        addr.sin_port = htons(port);
        freeaddrinfo(res);

        (void)emit_event(core::Event::info(core::EventType::DNS_RESOLVE_DONE, "Resolved: " + host));
        return util::ResultV<sockaddr_in>::Ok(addr);
    }

    util::ResultV<void>
    AsyncTCPClient::connect(
        const std::string &host,
        uint16_t port,
        int timeout_ms)
    {
        // 1. DNS 解析
        auto addr_res = resolve_host(host, port);
        if (addr_res.is_err())
            return util::ResultV<void>::Err(addr_res.unwrap_err());
        auto addr = addr_res.unwrap();

        // 2. 创建 Socket
        auto fd_res = platform::fd::Fd::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_res.is_err())
            return util::ResultV<void>::Err(fd_res.unwrap_err());
        sock = std::move(fd_res.unwrap());

        // 3. 设置非阻塞
        auto nb_res = set_nonblocking(sock.get());
        if (nb_res.is_err())
            return util::ResultV<void>::Err(nb_res.unwrap_err());

        // 4. 发起连接
        (void)emit_event(core::Event::info(core::EventType::TCP_CONNECT_START, "Connecting (non-blocking)...", sock.view()));

        int ret = ::connect(sock.get(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));

        if (ret < 0 && errno != EINPROGRESS)
        {
            auto err = util::Error::from_errno(errno, "Connect immediate failure");
            (void)emit_event(core::Event::failure(core::EventType::TCP_CONNECT_START, err, sock.view()));
            return util::ResultV<void>::Err(err);
        }

        if (ret == 0)
        {
            // 极少数情况：连接立即完成了
            (void)emit_event(core::Event::info(core::EventType::TCP_CONNECT_SUCCESS, "Connected immediately", sock.view()));
            return util::ResultV<void>::Ok();
        }

        // 5. 使用 Poller 等待连接完成 (等待可写)
        auto poller_res = platform::poller::Poller::create();
        if (poller_res.is_err())
            return util::ResultV<void>::Err(poller_res.unwrap_err());
        auto poller = std::move(poller_res.unwrap());

        // 监控可写(EPOLLOUT)和错误(EPOLLERR)
        (void)poller.add(sock, EPOLLOUT | EPOLLERR | EPOLLHUP);
        auto wait_res = poller.wait(timeout_ms);

        if (wait_res.is_err())
            return util::ResultV<void>::Err(wait_res.unwrap_err());
        auto events = wait_res.unwrap();

        if (events.empty())
        {
            auto err = util::Error::internal("Connection timeout");
            (void)emit_event(core::Event::failure(core::EventType::TCP_CONNECT_TIMEOUT, err, sock.view()));
            return util::ResultV<void>::Err(err);
        }

        // 6. 检查连接最终结果
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        if (getsockopt(sock.get(), SOL_SOCKET, SO_ERROR, &so_error, &len) < 0)
        {
            return util::ResultV<void>::Err(util::Error::from_errno(errno, "getsockopt SO_ERROR"));
        }

        if (so_error != 0)
        {
            auto err = util::Error::from_errno(so_error, "Async connect failed");
            (void)emit_event(core::Event::failure(core::EventType::TCP_CONNECT_START, err, sock.view()));
            return util::ResultV<void>::Err(err);
        }

        (void)emit_event(core::Event::info(core::EventType::TCP_CONNECT_SUCCESS, "Connected established", sock.view()));
        return util::ResultV<void>::Ok();
    }

    util::ResultV<size_t> AsyncTCPClient::send(const std::vector<std::byte> &data)
    {
        if (!sock)
            return util::ResultV<size_t>::Err(util::Error::internal("Socket not connected"));

        (void)emit_event(core::Event::info(core::EventType::HTTP_SENT, "Sending data...", sock.view()));

        ssize_t n = ::send(sock.get(), data.data(), data.size(), 0);
        if (n < 0)
        {
            auto err = util::Error::from_errno(errno, "Send failed");
            (void)emit_event(core::Event::failure(core::EventType::HTTP_SENT, err, sock.view()));
            return util::ResultV<size_t>::Err(err);
        }
        return util::ResultV<size_t>::Ok(static_cast<size_t>(n));
    }

    util::ResultV<size_t> AsyncTCPClient::recv(std::vector<std::byte> &buffer, size_t max_size)
    {
        // 简化版实现：实际生产中可能需要 Poller 等待可读
        buffer.resize(max_size);
        ssize_t n = ::recv(sock.get(), buffer.data(), max_size, 0);

        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return util::ResultV<size_t>::Ok(0);
            return util::ResultV<size_t>::Err(util::Error::from_errno(errno, "Recv failed"));
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
}