#ifndef INCLUDE_EUNET_UTIL_BYTE_BUFFER
#define INCLUDE_EUNET_UTIL_BYTE_BUFFER

#include <cstddef>
#include <vector>
#include <span>

namespace util
{
    class ByteBuffer;
    class ByteBufferView;

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
        void append(std::span<const std::byte> data);
        std::span<std::byte> prepare(size_t n);
        void commit(size_t n);

    public:
        std::span<const std::byte> readable() const;
        void consume(size_t n);

    public:
        void clear() noexcept;
        void reset();
        void compact();

    private:
        void ensure_writable(size_t n);
    };

}

#endif // INCLUDE_EUNET_UTIL_BYTE_BUFFER