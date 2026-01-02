#include "eunet/net/async_tcp_client.hpp"

#include "eunet/platform/net/dns_resolver.hpp"
#include "eunet/platform/net/endpoint.hpp"
#include "eunet/util/byte_buffer.hpp"
#include <fmt/format.h>

namespace net::tcp
{
    using util::Error;

    AsyncTCPClient::AsyncTCPClient(core::Orchestrator &o) : orch(o) {}

    // 析构时确保资源释放和事件上报
    AsyncTCPClient::~AsyncTCPClient() { close(); }

    util::ResultV<void> AsyncTCPClient::emit_event(const core::Event &e)
    {
        return orch.emit(e);
    }

    util::ResultV<void>
    AsyncTCPClient::connect(
        const std::string &host,
        uint16_t port,
        int timeout_ms)
    {
        using Ret = util::ResultV<void>;

        // 1. 发出 DNS 解析开始事件
        (void)emit_event(core::Event::info(core::EventType::DNS_RESOLVE_START, "Resolving host: " + host));

        // 2. 使用底层 DNSResolver
        auto resolve_res = platform::net::DNSResolver::resolve(
            host,
            port,
            platform::net::AddressFamily::IPv4 // 暂时默认 IPv4
        );

        if (resolve_res.is_err())
        {
            auto err = resolve_res.unwrap_err(); // 底层已经构造了精准的 Error::dns
            (void)emit_event(core::Event::failure(core::EventType::DNS_RESOLVE_DONE, err));
            return Ret::Err(err);
        }

        const auto &endpoints = resolve_res.unwrap();
        // 取第一个结果（生产环境可能需要尝试所有结果）
        const auto &target_ep = endpoints.front();

        (void)emit_event(core::Event::info(core::EventType::DNS_RESOLVE_DONE, "Resolved to: " + to_string(target_ep)));

        // 3. 创建 TCP Socket
        // 使用底层 TCPSocket::create，错误已封装好
        auto sock_res = platform::net::TCPSocket::create(platform::net::AddressFamily::IPv4);
        if (sock_res.is_err())
        {
            return Ret::Err(sock_res.unwrap_err());
        }

        // 暂存 socket，但尚未建立连接
        m_sock.emplace(std::move(sock_res.unwrap()));

        // 4. 发起连接
        (void)emit_event(core::Event::info(
            core::EventType::TCP_CONNECT_START,
            fmt::format("Connecting to {} (timeout={}ms)...", host, timeout_ms),
            m_sock->view()));

        // 调用底层封装的 connect (已包含 non-blocking + epoll wait 逻辑)
        auto conn_res = m_sock->connect(target_ep, timeout_ms);

        if (conn_res.is_err())
        {
            // 如果是超时，底层已经返回了 Error::transport().timeout()
            auto err = conn_res.unwrap_err();

            // 为错误添加更具体的上下文
            util::Error contextual_err =
                Error::transport()
                    .message("TCP connection failed")
                    .context(fmt::format("{}:{}", host, port))
                    .wrap(err) // 保留底层原因
                    .build();

            (void)emit_event(core::Event::failure(
                err.category() == util::ErrorCategory::Timeout
                    ? core::EventType::TCP_CONNECT_TIMEOUT
                    : core::EventType::TCP_CONNECT_START,
                contextual_err,
                m_sock->view()));

            // 连接失败，清理 socket
            m_sock.reset();
            return Ret::Err(contextual_err);
        }

        // 5. 连接成功
        (void)emit_event(core::Event::info(
            core::EventType::TCP_CONNECT_SUCCESS,
            "Connection established",
            m_sock->view()));

        return Ret::Ok();
    }

    util::ResultV<size_t>
    AsyncTCPClient::send(const std::vector<std::byte> &data)
    {
        using Ret = util::ResultV<size_t>;

        // 状态检查：使用 Error::state()
        if (!m_sock || !m_sock->is_open())
        {
            return Ret::Err(
                Error::state()
                    .invalid_state()
                    .message("Attempt to send on unconnected socket")
                    .context("AsyncTCPClient::send")
                    .build());
        }

        (void)emit_event(core::Event::info(
            core::EventType::HTTP_SENT,
            fmt::format("Sending {} bytes...", data.size()),
            m_sock->view()));

        // 适配接口：vector -> ByteBuffer
        // ByteBuffer 主要是为了 Zero-copy 设计的，但这里接口是 vector，所以必须拷贝一次
        // 如果追求极致性能，AsyncTCPClient 的接口应该改用 ByteBuffer
        util::ByteBuffer buf(data.size());
        buf.append(data);

        // 调用底层 write
        // timeout_ms = 0 表示非阻塞尝试写入，不做 wait
        // 如果需要保证写完，这里应该传一个超时时间
        auto write_res = m_sock->write(buf, 0);

        if (write_res.is_err())
        {
            auto err = write_res.unwrap_err();
            (void)emit_event(core::Event::failure(core::EventType::HTTP_SENT, err, m_sock->view()));
            return Ret::Err(err);
        }

        size_t n = write_res.unwrap();

        // 如果非阻塞写入导致没有发完（Partial Write），在业务层可能需要处理
        // 这里简单返回实际发送字节数
        return Ret::Ok(n);
    }

    util::ResultV<size_t>
    AsyncTCPClient::recv(
        std::vector<std::byte> &buffer,
        size_t max_size)
    {
        using Ret = util::ResultV<size_t>;

        if (!m_sock || !m_sock->is_open())
        {
            return Ret::Err(
                Error::state()
                    .invalid_state()
                    .message("Attempt to recv from unconnected socket")
                    .context("AsyncTCPClient::recv")
                    .build());
        }

        // 准备 ByteBuffer
        util::ByteBuffer internal_buf(max_size);

        // 调用底层 read
        // timeout_ms = 5000 (举例)，实际应该由参数传入或配置决定
        auto read_res = m_sock->read(internal_buf, 5000);

        if (read_res.is_err())
        {
            auto err = read_res.unwrap_err();

            // 忽略 EAGAIN/Timeout (视业务逻辑而定，这里假设超时是错误，或者返回0)
            if (err.category() == util::ErrorCategory::Timeout ||
                err.category() == util::ErrorCategory::Busy)
            {
                return Ret::Ok(0);
            }

            (void)emit_event(core::Event::failure(core::EventType::HTTP_RECEIVED, err, m_sock->view()));
            return Ret::Err(err);
        }

        size_t n = read_res.unwrap();

        if (n > 0)
        {
            auto readable = internal_buf.readable();
            buffer.resize(readable.size());
            std::memcpy(buffer.data(), readable.data(), readable.size());

            (void)emit_event(core::Event::info(
                core::EventType::HTTP_RECEIVED,
                fmt::format("Received {} bytes", n),
                m_sock->view()));
        }

        return Ret::Ok(n);
    }

    void AsyncTCPClient::close() noexcept
    {
        if (m_sock && m_sock->is_open())
        {
            (void)emit_event(core::Event::info(
                core::EventType::CONNECTION_CLOSED,
                "Closing connection",
                m_sock->view()));
            m_sock->close();
            // m_sock.reset() 不是必须的，因为 TCPSocket::close 已经释放了 FD，
            // 但 reset 可以让 m_sock 变回 empty 状态，逻辑更严谨
            m_sock.reset();
        }
    }
}