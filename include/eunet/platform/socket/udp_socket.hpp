#ifndef INCLUDE_EUNET_PLATFORM_SOCKET_UDP_SOCKET
#define INCLUDE_EUNET_PLATFORM_SOCKET_UDP_SOCKET

#include "eunet/util/error.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/net/common.hpp"
#include "eunet/platform/net/endpoint.hpp"
#include "eunet/platform/base_socket.hpp"

namespace platform::net
{
    class UDPSocket final
        : public BaseSocket
    {
    public:
        static util::ResultV<UDPSocket> create(
            AddressFamily af = AddressFamily::IPv4);

    public:
        explicit UDPSocket(fd::Fd &&fd) noexcept
            : BaseSocket(std::move(fd)) {}

        UDPSocket(const UDPSocket &) = delete;
        UDPSocket &operator=(const UDPSocket &) = delete;

        UDPSocket(UDPSocket &&) noexcept = default;
        UDPSocket &operator=(UDPSocket &&) noexcept = default;

    public:
        IOResult
        read(util::ByteBuffer &buf, int timeout_ms = -1) override;

        IOResult
        write(util::ByteBuffer &buf, int timeout_ms = -1) override;

        util::ResultV<void>
        connect(const Endpoint &ep, int timeout_ms = -1) override;
    };
}

#endif // INCLUDE_EUNET_PLATFORM_SOCKET_UDP_SOCKET
