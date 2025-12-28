#ifndef INCLUDE_EUNET_NET_IO_BUFFER
#define INCLUDE_EUNET_NET_IO_BUFFER

#include <vector>
#include <cstddef>
#include <algorithm>

namespace net
{
    /**
     * @brief 专门用于处理字节流的缓冲区。
     * 非线程安全（通常由网络线程独占），重点在于连续内存和减少移动。
     */
    class IOBuffer
    {
    private:
        std::vector<std::byte> buf;
        size_t m_read_idx;
        size_t m_write_idx;

    public:
        explicit IOBuffer(size_t init_size = 8192);

    public:
        // 可读字节数
        size_t readable_bytes() const;
        // 可写空间数
        size_t writable_bytes() const;
        // 获取读取指针
        const std::byte *peek() const;

    public:
        // 标记已读取
        void retrieve(size_t len);
        // 预留写空间
        void ensure_writable(size_t len);

    public:
        // 写入数据
        void append(const std::byte *data, size_t len);

        // 返回底层缓冲区的写位置指针，方便直接调用 ::recv
        std::byte *begin_write();

        void has_written(size_t len);

    private:
        void make_space(size_t len);
    };
}

#endif // INCLUDE_EUNET_NET_IO_BUFFER