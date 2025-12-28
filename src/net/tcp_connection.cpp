#include "eunet/net/tcp_connection.hpp"

namespace net::tcp
{
    TCPConnection::TCPConnection(platform::net::TCPSocket sock)
        : sock(std::move(sock)) {}

    util::ResultV<void>
    TCPConnection::send(
        const std::byte *data,
        size_t len,
        platform::time::Duration timeout)
    {
        // 1. 如果当前发送缓冲区没东西，尝试直接通过 Socket 发送，避免内存拷贝
        size_t written = 0;
        if (m_out_buffer.readable_bytes() == 0)
        {
            auto res = sock.send(data, len, timeout);
            if (res.is_ok())
                written = res.unwrap();
            else if (
                res.unwrap_err().code() != EAGAIN &&
                res.unwrap_err().code() != EWOULDBLOCK)
                return util::ResultV<void>::Err(res.unwrap_err());
        }

        // 2. 如果没发完（Socket 满了），把剩下的存进输出缓冲区
        if (written < len)
        {
            m_out_buffer.append(data + written, len - written);
        }

        return util::ResultV<void>::Ok();
    }

    util::ResultV<void>
    TCPConnection::send(
        const std::vector<std::byte> &data,
        platform::time::Duration timeout)
    {
        return send(data.data(), data.size(), timeout);
    }

    util::ResultV<void> TCPConnection::flush()
    {
        // 只要输出缓冲区里有数据，就不断尝试通过 Socket 发送
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
                    break; // Socket 暂时写不进去了
                m_out_buffer.retrieve(n);
            }
            else
            {
                // 如果是缓冲区满，停止发送，等待下次机会；否则返回错误
                if (res.unwrap_err().code() == EAGAIN ||
                    res.unwrap_err().code() == EWOULDBLOCK)
                    break;
                return util::ResultV<void>::Err(res.unwrap_err());
            }
        }
        return util::ResultV<void>::Ok();
    }

    util::ResultV<size_t> TCPConnection::read_available()
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

    void TCPConnection::close() { sock.close(); }
}