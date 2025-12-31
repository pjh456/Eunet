#ifndef INCLUDE_EUNET_PLATFORM_IO_CONTEXT
#define INCLUDE_EUNET_PLATFORM_IO_CONTEXT

#include <coroutine>
#include <unordered_map>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/util/byte_buffer.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/platform/poller.hpp"

namespace platform
{
    class IOContext;
    class ReadAwaitable;

    using IOResult = util::ResultV<size_t>;

    class IOContext
    {
    public:
        struct FdState
        {
            std::coroutine_handle<> read_waiter{};
        };

        using FdStateTable = std::unordered_map<int, FdState>;

    private:
        poller::Poller m_poller;

        FdStateTable m_states;
        bool m_running = false;

    public:
        static util::ResultV<IOContext> create();

    private:
        IOContext();

    public:
        ~IOContext() = default;

    public:
        void run();
        void stop();

    public:
        FdStateTable &fd_state() { return m_states; }
        const FdStateTable &fd_state() const { return m_states; }

        poller::Poller &poller() { return m_poller; }
        const poller::Poller &poller() const { return m_poller; }

    public:
        IOResult read(
            fd::Fd &fd,
            util::ByteBuffer &buf);
        IOResult write(
            fd::Fd &fd,
            util::ByteBuffer &buf);

    public:
        ReadAwaitable
        async_read(
            fd::Fd &fd,
            util::ByteBuffer &buf);
        // ReadAwaitable
        // async_write(
        //     fd::Fd &fd,
        //     util::ByteBuffer &buf);
    };

    class ReadAwaitable
    {
    private:
        IOContext &m_ctx;
        fd::Fd &m_fd;
        util::ByteBuffer &m_buf;
        IOResult result;

    public:
        ReadAwaitable(
            IOContext &ctx, fd::Fd &fd,
            util::ByteBuffer &buf);

    public:
        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> h);
        IOResult await_resume();
    };
}

#endif // INCLUDE_EUNET_PLATFORM_IO_CONTEXT