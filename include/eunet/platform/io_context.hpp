#ifndef INCLUDE_EUNET_PLATFORM_IO_CONTEXT
#define INCLUDE_EUNET_PLATFORM_IO_CONTEXT

#include <coroutine>
#include <unordered_map>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/util/byte_buffer.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/base_socket.hpp"
#include "eunet/platform/poller.hpp"

namespace platform
{
    class IOContext;
    class ReadAwaitable;

    using IOResult = util::ResultV<size_t>;

    class IOContext
    {
    private:
        poller::Poller m_poller;

    public:
        static util::ResultV<IOContext> create();

    private:
        IOContext();

    public:
        IOContext(const IOContext &) = delete;
        IOContext &operator=(const IOContext &) = delete;

        IOContext(IOContext &&) noexcept = default;
        IOContext &operator=(IOContext &&) noexcept = default;

        ~IOContext() = default;

    public:
        poller::Poller &poller() { return m_poller; }
        const poller::Poller &poller() const { return m_poller; }

    public:
        IOResult
        read(
            net::BaseSocket &sock,
            util::ByteBuffer &buf,
            time::Duration timeout);

        IOResult
        write(
            net::BaseSocket &sock,
            util::ByteBuffer &buf,
            time::Duration timeout);

    public:
        ReadAwaitable
        async_read(
            net::BaseSocket &sock,
            util::ByteBuffer &buf);
        // ReadAwaitable
        // async_write(
        //     fd::Fd &fd,
        //     util::ByteBuffer &buf);

    public:
        util::ResultV<void>
        wait_readable(
            net::BaseSocket &sock,
            int timeout_ms);

        ReadAwaitable
        async_wait_readable(
            net::BaseSocket &sock,
            util::ByteBuffer &buf);

    public:
        util::ResultV<void>
        wait_writable(
            net::BaseSocket &sock,
            int timeout_ms);

        // util::ResultV<void>
        // async_wait_writable(
        //     net::BaseSocket &sock,
        //     util::ByteBuffer &buf);
    };

    class ReadAwaitable
    {
    private:
        IOContext &m_ctx;
        fd::FdView m_fd;
        util::ByteBuffer &m_buf;
        IOResult result;

    public:
        ReadAwaitable(
            IOContext &ctx, fd::FdView fd,
            util::ByteBuffer &buf);

    public:
        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> h);
        IOResult await_resume();

    public:
        void set_result(IOResult r);
    };
}

#endif // INCLUDE_EUNET_PLATFORM_IO_CONTEXT