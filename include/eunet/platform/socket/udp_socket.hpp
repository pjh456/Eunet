/*
 * ============================================================================
 *  File Name   : udp_socket.hpp
 *  Module      : platform/net
 *
 *  Description :
 *      UDP 套接字的具体实现。继承自 BaseSocket，
 *      实现了数据报 (Datagram) 的 send/recv 逻辑，
 *      处理 UDP 特有的非连接特性。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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
            poller::Poller &poller,
            AddressFamily af = AddressFamily::IPv4);

    public:
        explicit UDPSocket(
            fd::Fd &&fd,
            poller::Poller &poller) noexcept
            : BaseSocket(std::move(fd), poller) {}

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
