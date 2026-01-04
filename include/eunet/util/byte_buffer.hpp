/*
 * ============================================================================
 *  File Name   : byte_buffer.hpp
 *  Module      : util
 *
 *  Description :
 *      动态字节缓冲区实现。提供类似 Netty ByteBuf 的读写指针管理，
 *      支持自动扩容、空间预留 (prepare) 和提交 (commit) 机制，
 *      用于非阻塞网络 IO 的数据缓冲。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_UTIL_BYTE_BUFFER
#define INCLUDE_EUNET_UTIL_BYTE_BUFFER

#include <cstddef>
#include <vector>
#include <span>

namespace util
{
    class ByteBuffer;
    class ByteBufferView;

    /**
     * @brief 动态字节缓冲区
     *
     * 提供类似 Netty ByteBuf 的读写指针分离机制。
     * 自动管理内存扩容，支持两阶段写入（Prepare -> Commit）和
     * 内存压缩（Compact）。
     *
     * @invariant 0 <= read_pos <= write_pos <= capacity
     */
    class ByteBuffer
    {
    private:
        std::vector<std::byte> m_storage;
        size_t m_read_pos = 0;
        size_t m_write_pos = 0;

        size_t m_pending_write = 0;

    public:
        explicit ByteBuffer(size_t cap = 0);

        ByteBuffer(const ByteBuffer &) = default;
        ByteBuffer &operator=(const ByteBuffer &) = default;

        ByteBuffer(ByteBuffer &&) noexcept = default;
        ByteBuffer &operator=(ByteBuffer &&) noexcept = default;

    public:
        size_t size() const noexcept;
        size_t capacity() const noexcept;
        size_t writable_size() const noexcept;
        bool empty() const noexcept;

    public:
        /**
         * @brief 向缓冲区末尾追加数据
         *
         * 如果剩余空间不足，会自动触发扩容。
         *
         * @param data 要写入的数据视图
         */
        void append(std::span<const std::byte> data);

        /**
         * @brief 准备写入空间（强校验）
         *
         * 返回一段可写的内存区域。必须随后调用 commit 确认写入。
         *
         * @throw std::logic_error 若此前已调用 prepare 但未 commit 就再次调用。
         *
         * @param n 需要写入的字节数
         * @return std::span<std::byte> 可写的内存视图
         */
        std::span<std::byte> prepare(size_t n);

        /**
         * @brief 准备写入空间（无校验）
         *
         * 返回一段可写的内存区域。必须随后调用 commit 确认写入。
         *
         * @param n 需要写入的字节数
         * @return std::span<std::byte> 可写的内存视图
         */
        std::span<std::byte> weak_prepare(size_t n);

        /**
         * @brief 提交写入（强校验）
         *
         * 更新 write_pos 指针，使 prepare 阶段写入的数据变为可读状态。
         *
         * @throw std::logic_error 若提交的字节数超出了实际申请的内存区域
         *
         * @param n 实际写入的字节数
         */
        void commit(size_t n);

        /**
         * @brief 提交写入（无校验）
         *
         * 更新 write_pos 指针，使 prepare 阶段写入的数据变为可读状态。
         *
         * @param n 实际写入的字节数
         */
        void weak_commit(size_t n);

    public:
        /**
         * @brief 获取可读空间
         *
         * 返回一段可读的内存区域。若希望读后消耗则需要手动调用 comsume。
         *
         * @return std::span<const std::byte> 可写的内存视图
         */
        std::span<const std::byte> readable() const;

        /**
         * @brief 提交已读
         *
         * 更新 read_pos 指针，使长度为 n 的字节流被消耗。
         *
         * @param n 实际已读消耗的字节数
         */
        void consume(size_t n);

    public:
        void clear() noexcept;
        void reset();

        /**
         * @brief 内存压缩
         *
         * 将 read_pos 到 write_pos 之间的未读数据移动到缓冲区头部，
         * 并重置 read_pos 为 0，从而腾出更多连续的尾部写入空间。
         */
        void compact();

    private:
        void ensure_writable(size_t n);
    };

}

#endif // INCLUDE_EUNET_UTIL_BYTE_BUFFER