/*
 * ============================================================================
 *  File Name   : tcp_socket.hpp
 *  Module      : platform/net
 *
 *  Description :
 *      TCP 套接字的具体实现。继承自 BaseSocket，
 *      实现了流式数据的 read/write 和面向连接的 connect 逻辑，
 *      集成了非阻塞模式下的 epoll 等待机制。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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