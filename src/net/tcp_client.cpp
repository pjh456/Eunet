#include "eunet/net/tcp_client.hpp"

#include "eunet/platform/net/dns_resolver.hpp"
#include "eunet/platform/net/endpoint.hpp"
#include "eunet/util/byte_buffer.hpp"
#include <fmt/format.h>

namespace net::tcp
{
    using util::Error;

    TCPClient::TCPClient(core::Orchestrator &o) : orch(o) {}

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

        (void)emit_event(
            core::Event::info(
                core::EventType::DNS_RESOLVE_START,
                "Resolving host: " + host));

        auto resolve_res =
            platform::net::DNSResolver::resolve(
                host, port,
                platform::net::AddressFamily::IPv4);

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

        const auto &ep = resolve_res.unwrap().front();

        (void)emit_event(
            core::Event::info(
                core::EventType::DNS_RESOLVE_DONE,
                "Resolved to: " + to_string(ep)));

        (void)emit_event(
            core::Event::info(
                core::EventType::TCP_CONNECT_START,
                fmt::format("Connecting to {}:{} (timeout={}ms)...",
                            host, port, timeout_ms)));

        auto conn_res = TCPConnection::connect(ep, timeout_ms);
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

        m_conn.emplace(std::move(conn_res.unwrap()));

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