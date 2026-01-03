#include "eunet/net/connection/udp_connection.hpp"

namespace net::udp
{
    util::ResultV<UDPConnection>
    UDPConnection::connect(
        const platform::net::Endpoint &ep,
        platform::poller::Poller &poller,
        int timeout_ms)
    {
        using platform::net::AddressFamily;

        auto af = static_cast<sa_family_t>(ep.family());
        auto domain =
            (af == AF_INET6)
                ? AddressFamily::IPv6
                : AddressFamily::IPv4;

        auto sock = platform::net::UDPSocket::create(poller, domain);
        if (sock.is_err())
            return util::ResultV<UDPConnection>::Err(
                sock.unwrap_err());

        auto s = std::move(sock.unwrap());

        /* 关键：先让内核分配本地端口 */
        auto bind_res = s.connect(platform::net::Endpoint::loopback_ipv4(0));
        if (bind_res.is_err())
            return util::ResultV<UDPConnection>::Err(
                bind_res.unwrap_err());

        auto res = s.connect(ep, timeout_ms);
        if (res.is_err())
            return util::ResultV<UDPConnection>::Err(
                res.unwrap_err());

        return util::ResultV<UDPConnection>::Ok(
            UDPConnection(std::move(s)));
    }

    UDPConnection
    UDPConnection::from_accepted_socket(
        platform::net::UDPSocket &&sock)
    {
        return UDPConnection(std::move(sock));
    }

    UDPConnection::UDPConnection(
        platform::net::UDPSocket &&sock) noexcept
        : m_sock(std::move(sock)) {}

    platform::fd::FdView
    UDPConnection::fd() const noexcept
    {
        return m_sock.view();
    }

    bool
    UDPConnection::is_open() const noexcept
    {
        return static_cast<bool>(m_sock.view());
    }

    void
    UDPConnection::close() noexcept
    {
        m_sock.close();
    }

    IOResult
    UDPConnection::read(
        util::ByteBuffer &buf,
        int timeout_ms)
    {
        size_t total_read = 0;

        // 1. 优先消费 in_buffer
        if (!m_in.empty())
        {
            auto readable = m_in.readable();
            buf.append(readable);
            total_read += readable.size();
            m_in.consume(readable.size());
            return IOResult::Ok(total_read);
        }

        // 2. 直接从 UDP socket 读取一个 datagram
        auto res = m_sock.read(buf, timeout_ms);
        if (res.is_err())
            return IOResult::Err(res.unwrap_err());

        total_read += res.unwrap();
        return IOResult::Ok(total_read);
    }

    IOResult
    UDPConnection::write(
        util::ByteBuffer &buf,
        int timeout_ms)
    {
        size_t total_written = 0;

        // UDP 不适合“流式直写 + 余量缓存”
        // 这里的策略是：
        // - 如果 out_buffer 为空，尝试一次性发送
        // - 失败或非阻塞未写完，则进入 out_buffer

        if (m_out.empty())
        {
            auto readable = buf.readable();
            if (!readable.empty())
            {
                auto res = m_sock.write(buf, timeout_ms);
                if (res.is_err())
                    return IOResult::Err(res.unwrap_err());

                total_written += res.unwrap();
            }
        }

        // 剩余内容进入 out_buffer
        if (!buf.readable().empty())
        {
            auto rem = buf.readable();
            m_out.append(rem);
            total_written += rem.size();
            buf.consume(rem.size());
        }

        return IOResult::Ok(total_written);
    }

    bool
    UDPConnection::has_pending_output() const noexcept
    {
        return !m_out.empty();
    }

    util::ResultV<void>
    UDPConnection::flush()
    {
        if (m_out.empty())
            return util::ResultV<void>::Ok();

        // UDP flush 只尝试一次
        auto res = m_sock.write(
            m_out,
            0 /* non-blocking */);

        if (res.is_err())
            return util::ResultV<void>::Err(
                res.unwrap_err());

        return util::ResultV<void>::Ok();
    }
}
