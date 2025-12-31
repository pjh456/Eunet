#include "eunet/util/byte_buffer.hpp"

#include <cstring>
#include <stdexcept>

namespace util
{
    ByteBuffer::ByteBuffer(size_t cap)
    {
        m_storage.resize(cap);
    }

    size_t ByteBuffer::size() const noexcept { return m_write_pos - m_read_pos; }
    size_t ByteBuffer::capacity() const noexcept { return m_storage.size(); }
    size_t ByteBuffer::writable_size() const noexcept { return m_storage.size() - m_write_pos; }
    bool ByteBuffer::empty() const noexcept { return size() == 0; }

    void ByteBuffer::append(
        std::span<const std::byte> data)
    {
        ensure_writable(data.size());
        std::memcpy(
            m_storage.data() + m_write_pos,
            data.data(),
            data.size());
        m_write_pos += data.size();
    }

    std::span<std::byte>
    ByteBuffer::prepare(size_t n)
    {
        ensure_writable(n);
        return {m_storage.data() + m_write_pos, n};
    }
    void ByteBuffer::commit(size_t n)
    {
        if (m_write_pos + n > m_storage.size())
            throw std::out_of_range("ByteBuffer::commit: Size is out of range.");
        m_write_pos += n;
    }

    std::span<const std::byte>
    ByteBuffer::readable()
        const { return {m_storage.data() + m_read_pos, size()}; }

    void ByteBuffer::consume(size_t n)
    {
        if (n > size())
            throw std::out_of_range("ByteBuffer::consume: Size is out of range.");
        m_read_pos += n;

        compact();
    }

    void ByteBuffer::clear() noexcept { m_read_pos = m_write_pos = 0; }
    void ByteBuffer::reset()
    {
        m_storage.clear();
        clear();
    }

    void ByteBuffer::compact()
    {
        if (m_read_pos == 0)
            return;

        const size_t sz = size();
        std::memmove(
            m_storage.data(),
            m_storage.data() + m_read_pos,
            sz);
        m_read_pos = 0;
        m_write_pos = sz;
    }

    void ByteBuffer::ensure_writable(size_t n)
    {
        if (m_write_pos + n <= m_storage.size())
            return;
        compact();
        if (m_write_pos + n <= m_storage.size())
            return;
        const size_t new_cap =
            std::max(m_storage.size() * 2, m_write_pos + n);
        m_storage.resize(new_cap);
    }
}