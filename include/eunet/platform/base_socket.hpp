#ifndef INCLUDE_EUNET_PLATFORM_BASE_SOCKET
#define INCLUDE_EUNET_PLATFORM_BASE_SOCKET

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/platform/io_context.hpp"
#include "eunet/platform/net/endpoint.hpp"

namespace platform::net
{
    class NonBlockingGuard
    {
    private:
        fd::FdView m_fd;
        int m_old_flags;

    public:
        explicit NonBlockingGuard(fd::FdView fd) noexcept;
        ~NonBlockingGuard() noexcept;

        NonBlockingGuard(const NonBlockingGuard &) = delete;
        NonBlockingGuard &operator=(const NonBlockingGuard &) = delete;
    };

    class BaseSocket
    {
    protected:
        fd::Fd m_fd;

        explicit BaseSocket(fd::Fd &&fd);

    public:
        BaseSocket(const BaseSocket &) = delete;
        BaseSocket &operator=(const BaseSocket &) = delete;

        BaseSocket(BaseSocket &&) noexcept = default;
        BaseSocket &operator=(BaseSocket &&) noexcept = default;

        ~BaseSocket() = default;

    public:
        fd::FdView view() const noexcept;
        bool is_open() const noexcept;
        void close() noexcept;

    public:
        virtual IOResult
        try_read(util::ByteBuffer &buf) = 0;

        virtual IOResult
        try_write(util::ByteBuffer &buf) = 0;
    };
}

#endif // INCLUDE_EUNET_PLATFORM_BASE_SOCKET