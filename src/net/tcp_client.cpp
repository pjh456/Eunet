/*
 * ============================================================================
 *  File Name   : tcp_client.cpp
 *  Module      : net/tcp
 *
 *  Description :
 *      TCP 客户端逻辑实现。串联 DNS 解析 -> 建立 TCP 连接 -> 发送/接收数据
 *      的完整流程，并在每一步产生对应的 Timeline 事件。
 *
 *  Third-Party Dependencies :
 *      - fmt
 *          Usage     : 格式化事件日志
 *          License   : MIT License
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#include "eunet/net/tcp_client.hpp"

#include "eunet/platform/net/dns_resolver.hpp"
#include "eunet/platform/net/endpoint.hpp"
#include "eunet/util/byte_buffer.hpp"
#include <fmt/format.h>

namespace net::tcp
{
    using util::Error;

    TCPClient::TCPClient(core::Orchestrator &o)
        : orch(o),
          m_poller(platform::poller::Poller::create().unwrap()) {}

    // 析构时确保资源释放和事件上报
    TCPClient::~TCPClient() { close(); }

    util::ResultV<void> TCPClient::emit_event(const core::Event &e)
    {
        return orch.emit(e);
    }

    util::ResultV<void>
    TCPClient::connect(
        const std::string &host,
        uint16_t port,
        int timeout_ms)
    {
        using Ret = util::ResultV<void>;
        using util::Error;

        // 上报 DNS 解析开始事件
        (void)emit_event(
            core::Event::info(
                core::EventType::DNS_RESOLVE_START,
                "Resolving host: " + host));

        // 调用底层 Resolver 进行域名解析
        auto resolve_res =
            platform::net::DNSResolver::resolve(
                host, port,
                platform::net::AddressFamily::IPv4);

        // 检查解析结果 如果失败则上报 DNS 解析失败事件并返回
        if (resolve_res.is_err())
        {
            auto err = resolve_res.unwrap_err();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::DNS_RESOLVE_DONE,
                    err));

            return Ret::Err(
                Error::dns()
                    .message("DNS resolve failed")
                    .context("TCPClient::connect")
                    .wrap(err)
                    .build());
        }

        // 获取解析到的第一个 Endpoint
        const auto &ep = resolve_res.unwrap().front();

        // 上报 DNS 解析完成事件
        (void)emit_event(
            core::Event::info(
                core::EventType::DNS_RESOLVE_DONE,
                "Resolved to: " + to_string(ep)));

        // 上报 TCP 连接开始事件
        (void)emit_event(
            core::Event::info(
                core::EventType::TCP_CONNECT_START,
                fmt::format("Connecting to {}:{} (timeout={}ms)...",
                            host, port, timeout_ms)));

        // 调用底层 TCP Connection 的连接逻辑
        auto conn_res = TCPConnection::connect(ep, m_poller, timeout_ms);

        // 检查连接结果 如果失败则上报连接失败事件
        if (conn_res.is_err())
        {
            auto err = conn_res.unwrap_err();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::TCP_CONNECT_START,
                    err));

            return Ret::Err(
                Error::transport()
                    .message("TCP connect failed")
                    .context("TCPClient::connect")
                    .wrap(err)
                    .build());
        }

        // 保存连接对象所有权
        m_conn.emplace(std::move(conn_res.unwrap()));

        // 上报 TCP 连接成功事件 附带分配的 FD
        (void)emit_event(
            core::Event::info(
                core::EventType::TCP_CONNECT_SUCCESS,
                "Connection established",
                m_conn->fd()));

        return Ret::Ok();
    }

    util::ResultV<size_t>
    TCPClient::send(const std::vector<std::byte> &data)
    {
        using Ret = util::ResultV<size_t>;
        using util::Error;

        if (!m_conn || !m_conn->is_open())
        {
            auto err = Error::state()
                           .invalid_state()
                           .message("send on unconnected")
                           .context("TCPClient::send")
                           .build();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::HTTP_SENT,
                    err));

            return Ret::Err(err);
        }

        (void)emit_event(
            core::Event::info(
                core::EventType::HTTP_SENT,
                fmt::format("Sending {} bytes...", data.size()),
                m_conn->fd()));

        util::ByteBuffer buf(data.size());
        buf.append(data);

        auto res = m_conn->write(buf, 0);
        if (res.is_err())
        {
            auto err = res.unwrap_err();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::HTTP_SENT,
                    err, m_conn->fd()));

            return Ret::Err(
                Error::transport()
                    .message("TCP send failed")
                    .context("TCPClient::send")
                    .wrap(err)
                    .build());
        }

        auto flush_res = m_conn->flush();
        if (flush_res.is_err())
        {
            auto err = flush_res.unwrap_err();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::HTTP_SENT,
                    err, m_conn->fd()));

            return Ret::Err(
                Error::transport()
                    .message("flush connection failed")
                    .context("TCPClient::send")
                    .wrap(err)
                    .build());
        }

        (void)emit_event(
            core::Event::info(
                core::EventType::HTTP_SENT,
                fmt::format("Sent {} bytes", data.size()),
                m_conn->fd()));

        return Ret::Ok(data.size());
    }

    util::ResultV<size_t>
    TCPClient::recv(
        std::vector<std::byte> &buffer,
        size_t max_size)
    {
        using Ret = util::ResultV<size_t>;
        using util::Error;

        if (!m_conn || !m_conn->is_open())
        {
            auto err = Error::state()
                           .invalid_state()
                           .message("recv on unconnected")
                           .context("TCPClient::recv")
                           .build();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::HTTP_RECEIVED,
                    err));

            return Ret::Err(err);
        }

        util::ByteBuffer buf(max_size);

        auto read_res = m_conn->read(buf, 3000);
        if (read_res.is_err())
        {
            auto err = read_res.unwrap_err();

            (void)emit_event(
                core::Event::failure(
                    core::EventType::HTTP_RECEIVED,
                    err, m_conn->fd()));

            if (err.category() == util::ErrorCategory::PeerClosed)
                return Ret::Err(err);

            return Ret::Err(
                Error::transport()
                    .message("connection recv failed")
                    .context("TCPClient::recv")
                    .wrap(err)
                    .build());
        }

        size_t n = read_res.unwrap();

        if (n > 0)
        {
            auto readable = buf.readable();
            buffer.resize(readable.size());
            std::memcpy(buffer.data(), readable.data(), readable.size());

            (void)emit_event(
                core::Event::info(
                    core::EventType::HTTP_RECEIVED,
                    fmt::format("Received {} bytes", n),
                    m_conn->fd()));
        }

        return Ret::Ok(n);
    }

    void TCPClient::close() noexcept
    {
        if (m_conn && m_conn->is_open())
        {
            (void)emit_event(
                core::Event::info(
                    core::EventType::CONNECTION_CLOSED,
                    "Closing connection",
                    m_conn->fd()));
            m_conn->close();
            m_conn.reset();
        }
    }
}