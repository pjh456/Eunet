#ifndef INCLUDE_EUNET_NET_CONNECTION
#define INCLUDE_EUNET_NET_CONNECTION

#include <span>
#include <cstddef>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/util/byte_buffer.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/platform/time.hpp"

namespace net
{
    using IOResult = util::ResultV<size_t>;
    class Connection
    {
    public:
        virtual ~Connection() = default;

    public:
        // --- 生命周期 ---
        virtual platform::fd::FdView fd() const noexcept = 0;
        virtual bool is_open() const noexcept = 0;
        virtual void close() noexcept = 0;

    public:
        // --- 核心 IO ---
        // 从连接读取数据，写入 buf
        virtual IOResult
        read(util::ByteBuffer &buf,
             int timeout_ms = -1) = 0;

        // 从 buf 写入连接
        virtual IOResult
        write(util::ByteBuffer &buf,
              int timeout_ms = -1) = 0;

    public:
        // --- 可选语义 ---
        // 是否还有待发送的数据
        virtual bool has_pending_output() const noexcept { return false; }

        // 强制刷新输出缓冲
        virtual util::ResultV<void> flush() { return util::ResultV<void>::Ok(); }
    };

}

#endif // INCLUDE_EUNET_NET_CONNECTION