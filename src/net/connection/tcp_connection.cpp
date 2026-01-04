/*
 * ============================================================================
 *  File Name   : tcp_connection.cpp
 *  Module      : net/tcp
 *
 *  Description :
 *      TCP Connection 实现。在 Socket 与 ByteBuffer 之间搬运数据，
 *      提供 flush 机制确保数据在非阻塞 Socket 上尽可能写出。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#include "eunet/net/connection/tcp_connection.hpp"

#include <utility>
#include <algorithm>
#include <string.h>

namespace net::tcp
{
    util::ResultV<TCPConnection>
    TCPConnection::connect(
        const platform::net::Endpoint &ep,
        platform::poller::Poller &poller,
        int timeout_ms)
    {
        using platform::net::AddressFamily;

        auto af = static_cast<sa_family_t>(ep.family());
        auto domain = (af == AF_INET6) ? AddressFamily::IPv6 : AddressFamily::IPv4;

        auto sock = platform::net::TCPSocket::create(poller, domain);
        if (sock.is_err())
            return util::ResultV<TCPConnection>::Err(sock.unwrap_err());

        auto s = std::move(sock.unwrap());

        auto res = s.connect(ep, timeout_ms);
        if (res.is_err())
            return util::ResultV<TCPConnection>::Err(res.unwrap_err());

        return util::ResultV<TCPConnection>::Ok(
            TCPConnection(std::move(s)));
    }

    TCPConnection::TCPConnection(
        platform::net::TCPSocket &&sock) noexcept
        : m_sock(std::move(sock)) {}

    TCPConnection
    TCPConnection::from_accepted_socket(
        platform::net::TCPSocket &&sock)
    {
        return TCPConnection(std::move(sock));
    }

    platform::fd::FdView
    TCPConnection::fd() const noexcept
    {
        return m_sock.view();
    }

    bool TCPConnection::is_open() const noexcept
    {
        return static_cast<bool>(m_sock.view());
    }

    void TCPConnection::close() noexcept
    {
        m_sock.close();
    }

    IOResult
    TCPConnection::read(
        util::ByteBuffer &buf,
        int timeout_ms)
    {
        size_t total_read = 0;

        // 1. 先搬已有的 in_buffer
        if (!m_in.empty())
        {
            auto readable = m_in.readable();
            buf.append(readable);
            total_read += readable.size();
            m_in.consume(readable.size());
            return IOResult::Ok(total_read);
        }

        // 2. 直接从 socket 读进 buf
        auto res = m_sock.read(buf, timeout_ms);

        if (res.is_err())
            return IOResult::Err(res.unwrap_err());

        size_t n = res.unwrap();
        total_read += n;

        return IOResult::Ok(total_read);
    }

    IOResult
    TCPConnection::write(
        util::ByteBuffer &buf,
        int timeout_ms)
    {
        size_t total_written = 0;

        // 1. 如果没有积压，尝试直写 socket
        if (m_out.empty())
        {
            auto readable = buf.readable();
            if (!readable.empty())
            {
                auto res = m_sock.write(
                    buf,
                    timeout_ms);

                if (res.is_err())
                    return IOResult::Err(res.unwrap_err());

                size_t n = res.unwrap();
                total_written += n;
            }
        }

        // 2. 剩余的全部进入 out_buffer
        if (!buf.readable().empty())
        {
            auto rem = buf.readable();
            m_out.append(rem);
            total_written += rem.size();
            buf.consume(rem.size());
        }

        return IOResult::Ok(total_written);
    }

    bool TCPConnection::has_pending_output() const noexcept
    {
        return !m_out.empty();
    }

    util::ResultV<void>
    TCPConnection::flush()
    {
        while (m_out.size() > 0)
        {
            auto res = m_sock.write(
                m_out,
                0 /* non-blocking */);

            if (res.is_err())
                return util::ResultV<void>::Err(res.unwrap_err());

            if (res.unwrap() == 0)
                break;
        }
        return util::ResultV<void>::Ok();
    }
}