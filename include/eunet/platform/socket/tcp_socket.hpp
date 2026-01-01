#ifndef INCLUDE_EUNET_PLATFORM_SOCKET_TCP_SOCKET
#define INCLUDE_EUNET_PLATFORM_SOCKET_TCP_SOCKET

#include "eunet/platform/time.hpp"
#include "eunet/platform/base_socket.hpp"
#include "eunet/platform/net/common.hpp"
#include "eunet/platform/net/endpoint.hpp"

namespace platform::net
{
    class TCPSocket final
        : public BaseSocket
    {
    public:
        static util::ResultV<TCPSocket> create(
            AddressFamily af = AddressFamily::IPv4);

    public:
        explicit TCPSocket(platform::fd::Fd &&fd) noexcept
            : BaseSocket(std::move(fd)) {}

        TCPSocket(const TCPSocket &) = delete;
        TCPSocket &operator=(const TCPSocket &) = delete;

        TCPSocket(TCPSocket &&) noexcept = default;
        TCPSocket &operator=(TCPSocket &&) noexcept = default;

    public:
        IOResult
        try_read(util::ByteBuffer &buf) override;

        IOResult
        try_write(util::ByteBuffer &buf) override;

        util::ResultV<void>
        try_connect(const Endpoint &ep) override;
    };
}
#endif // INCLUDE_EUNET_PLATFORM_SOCKET_TCP_SOCKET