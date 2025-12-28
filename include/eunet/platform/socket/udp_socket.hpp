#ifndef INCLUDE_EUNET_PLATFORM_SOCKET_UDP_SOCKET
#define INCLUDE_EUNET_PLATFORM_SOCKET_UDP_SOCKET

#include "eunet/util/error.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/address.hpp"
#include "eunet/platform/socket_base.hpp"

namespace platform::net
{
    class UDPSocket final : public SocketBase
    {
    public:
        static util::ResultV<UDPSocket>
        create(AddressFamily af = AddressFamily::IPv4);

    public:
        explicit UDPSocket(fd::Fd &&fd) noexcept;

    public:
        util::ResultV<void>
        connect(const SocketAddress &peer);

        util::ResultV<size_t>
        send(const std::byte *data, size_t len);

        util::ResultV<size_t>
        recv(std::byte *buf, size_t len);
    };
}

#endif // INCLUDE_EUNET_PLATFORM_SOCKET_UDP_SOCKET
