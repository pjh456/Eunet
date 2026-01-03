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
            poller::Poller &poller,
            AddressFamily af = AddressFamily::IPv4);

    public:
        explicit TCPSocket(
            platform::fd::Fd &&fd,
            poller::Poller &poller) noexcept
            : BaseSocket(std::move(fd), poller) {}

        TCPSocket(const TCPSocket &) = delete;
        TCPSocket &operator=(const TCPSocket &) = delete;

        TCPSocket(TCPSocket &&) noexcept = default;
        TCPSocket &operator=(TCPSocket &&) noexcept = default;

    public:
        IOResult
        read(util::ByteBuffer &buf, int timeout_ms = -1) override;

        IOResult
        write(util::ByteBuffer &buf, int timeout_ms = -1) override;

        util::ResultV<void>
        connect(const Endpoint &ep, int timeout_ms = -1) override;
    };
}
#endif // INCLUDE_EUNET_PLATFORM_SOCKET_TCP_SOCKET