#ifndef INCLUDE_EUNET_PLATFORM_SOCKET_TCP_SOCKET
#define INCLUDE_EUNET_PLATFORM_SOCKET_TCP_SOCKET

#include "eunet/platform/socket_base.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/address.hpp"

namespace platform::net
{
    class TCPSocket final : public SocketBase
    {
    public:
        static util::ResultV<TCPSocket> create();

    public:
        explicit TCPSocket(fd::Fd fd);

    public:
        util::ResultV<void>
        connect(
            const SocketAddress &addr,
            time::Duration timeout);

        util::ResultV<size_t>
        send(
            const std::byte *data,
            size_t len,
            time::Duration timeout);

        util::ResultV<size_t>
        recv(
            std::byte *buf,
            size_t len,
            time::Duration timeout);
    };
}
#endif // INCLUDE_EUNET_PLATFORM_SOCKET_TCP_SOCKET