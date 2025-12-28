#include "eunet/net/connection/tcp_connection.hpp"

#include <utility>
#include <algorithm>
#include <string.h>

namespace net::tcp
{
    util::ResultV<TCPConnection>
    TCPConnection::connect(
        const platform::net::SocketAddress &addr,
        platform::time::Duration timeout)
    {
        using Result = util::ResultV<TCPConnection>;

        auto sock = platform::net::TCPSocket::create();
        if (sock.is_err())
            return Result::Err(sock.unwrap_err());

        auto res = sock.unwrap().connect(addr, timeout);
        if (res.is_err())
            return Result::Err(res.unwrap_err());

        return Result::Ok(TCPConnection(std::move(sock.unwrap())));
    }

    TCPConnection
    TCPConnection::from_accepted_socket(
        platform::net::TCPSocket &&sock)
    {
        return TCPConnection(std::move(sock));
    }

    TCPConnection::TCPConnection(
        platform::net::TCPSocket &&sock)
        : sock(std::move(sock)) {}

    platform::fd::FdView
    TCPConnection::fd() const noexcept { return sock.view(); }

    void TCPConnection::close() noexcept { sock.close(); }

    bool TCPConnection::is_open() const noexcept { return (bool)sock.view(); }

    bool TCPConnection::has_pending_output()
        const noexcept { return m_out_buffer.readable_bytes() > 0; }

    util::ResultV<void>
    TCPConnection::flush()
    {
        while (m_out_buffer.readable_bytes() > 0)
        {
            platform::time::Duration timeout(50);
            auto res = sock.send(
                m_out_buffer.peek(),
                m_out_buffer.readable_bytes(),
                timeout);

            if (res.is_ok())
            {
                size_t n = res.unwrap();
                if (n == 0)
                    break;
                m_out_buffer.retrieve(n);
            }
            else
            {
                if (res.unwrap_err().code() == EAGAIN ||
                    res.unwrap_err().code() == EWOULDBLOCK)
                    break;
                return util::ResultV<void>::Err(res.unwrap_err());
            }
        }
        return util::ResultV<void>::Ok();
    }

    util::ResultV<size_t>
    TCPConnection::write(
        const std::byte *data,
        size_t len,
        platform::time::Duration timeout)
    {
        size_t written = 0;

        // 1. 优先直写 socket
        if (m_out_buffer.readable_bytes() == 0)
        {
            auto res = sock.send(data, len, timeout);
            if (res.is_ok())
                written = res.unwrap();
            else if (
                res.unwrap_err().code() != EAGAIN &&
                res.unwrap_err().code() != EWOULDBLOCK)
                return util::ResultV<size_t>::Err(res.unwrap_err());
        }

        // 2. 剩余进入 buffer
        if (written < len)
            m_out_buffer.append(data + written, len - written);

        return util::ResultV<size_t>::Ok(len);
    }

    util::ResultV<size_t>
    TCPConnection::read(
        std::byte *buf,
        size_t len,
        platform::time::Duration timeout)
    {
        // 1. buffer 有数据，优先读 buffer
        if (m_in_buffer.readable_bytes() > 0)
        {
            size_t n = std::min(len, m_in_buffer.readable_bytes());
            memcpy(buf, m_in_buffer.peek(), n);
            m_in_buffer.retrieve(n);
            return util::ResultV<size_t>::Ok(n);
        }

        // 2. 直接从 socket 读
        auto res = sock.recv(buf, len, timeout);
        if (res.is_ok())
            return util::ResultV<size_t>::Ok(res.unwrap());

        return util::ResultV<size_t>::Err(res.unwrap_err());
    }

    IOBuffer &
    TCPConnection::in_buffer() { return m_in_buffer; }

    IOBuffer &
    TCPConnection::out_buffer() { return m_out_buffer; }

    util::ResultV<size_t>
    TCPConnection::read_available()
    {
        size_t total_read = 0;
        while (true)
        {
            // 预留 4KB 写空间
            m_in_buffer.ensure_writable(4096);

            // 直接让 Socket 把数据写进 IOBuffer 的写空闲区
            platform::time::Duration timeout(50);
            auto res = sock.recv(
                m_in_buffer.begin_write(),
                m_in_buffer.writable_bytes(),
                timeout);

            if (res.is_ok())
            {
                size_t n = res.unwrap();
                if (n == 0)
                    break; // 对端关闭或当前没数据了

                m_in_buffer.has_written(n);
                total_read += n;
            }
            else
            {
                // 非阻塞下读完了
                if (res.unwrap_err().code() == EAGAIN || res.unwrap_err().code() == EWOULDBLOCK)
                    break;
                return util::ResultV<size_t>::Err(res.unwrap_err());
            }
        }
        return util::ResultV<size_t>::Ok(total_read);
    }
}