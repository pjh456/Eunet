#ifndef INCLUDE_EUNET_PLATFORM_SOCKET_BASE
#define INCLUDE_EUNET_PLATFORM_SOCKET_BASE

#include "eunet/platform/fd.hpp"
#include "eunet/platform/address.hpp"
#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"

namespace platform::net
{
    class NonBlockingGuard
    {
    public:
        explicit NonBlockingGuard(fd::FdView fd) noexcept;
        ~NonBlockingGuard() noexcept;

        NonBlockingGuard(const NonBlockingGuard &) = delete;
        NonBlockingGuard &operator=(const NonBlockingGuard &) = delete;

    private:
        fd::FdView m_fd;
        int m_old_flags;
    };

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
        bool is_open() const noexcept;
        void close() noexcept;

        util::ResultV<SocketAddress> local_address() const;
        util::ResultV<SocketAddress> peer_address() const;
    };
}

#endif // INCLUDE_EUNET_PLATFORM_SOCKET_BASE