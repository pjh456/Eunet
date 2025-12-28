#include "eunet/net/io_buffer.hpp"

#include <algorithm>

namespace net
{
    IOBuffer::IOBuffer(size_t init_size)
        : buf(init_size),
          m_read_idx(0), m_write_idx(0) {}

    size_t IOBuffer::readable_bytes()
        const { return m_write_idx - m_read_idx; }

    size_t IOBuffer::writable_bytes()
        const { return buf.size() - m_write_idx; }

    const std::byte *
    IOBuffer::peek()
        const { return buf.data() + m_read_idx; }

    void IOBuffer::retrieve(
        size_t len)
    {
        m_read_idx += len;
        if (m_read_idx == m_write_idx)
            m_read_idx = m_write_idx = 0;
    }

    void IOBuffer::ensure_writable(
        size_t len)
    {
        if (writable_bytes() < len)
            make_space(len);
    }

    void IOBuffer::append(
        const std::byte *data,
        size_t len)
    {
        ensure_writable(len);
        std::copy(data, data + len, buf.data() + m_write_idx);
        m_write_idx += len;
    }

    std::byte *IOBuffer::begin_write() { return buf.data() + m_write_idx; }

    void IOBuffer::has_written(size_t len) { m_write_idx += len; }

    void IOBuffer::make_space(size_t len)
    {
        if (writable_bytes() + m_read_idx < len)
            buf.resize(m_write_idx + len);
        else
        {
            size_t readable = readable_bytes();
            std::copy(buf.data() + m_read_idx, buf.data() + m_write_idx, buf.data());
            m_read_idx = 0;
            m_write_idx = readable;
        }
    }
}