#ifndef INCLUDE_EUNET_PLATFORM_SOCKET_BASE
#define INCLUDE_EUNET_PLATFORM_SOCKET_BASE

#include "eunet/platform/fd.hpp"

namespace platform::net
{
    class SocketBase
    {
    protected:
        fd::Fd m_fd;

    protected:
        explicit SocketBase(fd::Fd fd);

    public:
        SocketBase(const SocketBase &) = delete;
        SocketBase &operator=(const SocketBase &) = delete;

        SocketBase(SocketBase &&) noexcept = default;
        SocketBase &operator=(SocketBase &&) noexcept = default;

        virtual ~SocketBase() = default;

    public:
        fd::FdView view() const noexcept;

        void set_nonblocking(bool enable) noexcept;
        void close() noexcept;
    };
}

#endif // INCLUDE_EUNET_PLATFORM_SOCKET_BASE