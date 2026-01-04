/*
 * ============================================================================
 *  File Name   : byte_buffer.cpp
 *  Module      : util
 *
 *  Description :
 *      ByteBuffer 的实现代码。包含内存移动 (compact)、扩容 (ensure_writable)
 *      以及读写指针的具体操作逻辑。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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
        if (m_pending_write != 0)
            throw std::logic_error("prepare called twice without commit");

        ensure_writable(n);
        m_pending_write = n;
        return {m_storage.data() + m_write_pos, n};
    }
    std::span<std::byte>
    ByteBuffer::weak_prepare(size_t n)
    {
        if (m_pending_write != 0)
            throw std::logic_error("prepare called twice without commit");

        ensure_writable(n);
        return {m_storage.data() + m_write_pos, n};
    }

    void ByteBuffer::commit(size_t n)
    {
        if (n > m_pending_write)
            throw std::logic_error("commit more than prepared");

        m_write_pos += n;
        m_pending_write -= n;
    }

    void ByteBuffer::weak_commit(size_t n)
    {
        if (n > writable_size())
            throw std::logic_error("commit more than prepared");

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
        // 检查读取位置是否已经在头部 如果是则无需移动
        if (m_read_pos == 0)
            return;

        // 计算当前未读数据的有效长度
        const size_t sz = size();

        // 使用 memmove 将有效数据搬运到缓冲区起始位置
        // 注意此处必须用 memmove 而非 memcpy 因为内存区域可能重叠
        std::memmove(
            m_storage.data(),
            m_storage.data() + m_read_pos,
            sz);

        // 重置读取指针为 0
        m_read_pos = 0;

        // 更新写入指针为有效数据长度
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