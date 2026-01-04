/*
 * ============================================================================
 *  File Name   : udp_connection.hpp
 *  Module      : net/udp
 *
 *  Description :
 *      UDP 连接的高级封装。尽管 UDP 无连接，但此类抽象了“到一个特定对端”
 *      的通信上下文，持有 UDPSocket 和缓冲区，统一了 IO 接口。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_NET_CONNECTION_UDP_CONNECTION
#define INCLUDE_EUNET_NET_CONNECTION_UDP_CONNECTION

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/util/byte_buffer.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/net/endpoint.hpp"
#include "eunet/platform/socket/udp_socket.hpp"
#include "eunet/net/connection.hpp"

namespace net::udp
{
    class UDPConnection;

    class UDPConnection final : public Connection
    {
    private:
        platform::net::UDPSocket m_sock;
        util::ByteBuffer m_in;
        util::ByteBuffer m_out;

    public:
        static util::ResultV<UDPConnection>
        connect(const platform::net::Endpoint &ep,
                platform::poller::Poller &poller,
                int timeout_ms = -1);

        static UDPConnection
        from_accepted_socket(platform::net::UDPSocket &&sock);

    public:
        explicit UDPConnection(platform::net::UDPSocket &&sock) noexcept;

        UDPConnection(const UDPConnection &) = delete;
        UDPConnection &operator=(const UDPConnection &) = delete;

        UDPConnection(UDPConnection &&) noexcept = default;
        UDPConnection &operator=(UDPConnection &&) noexcept = default;

    public:
        platform::fd::FdView fd() const noexcept override;
        bool is_open() const noexcept override;
        void close() noexcept override;

    public:
        IOResult
        read(util::ByteBuffer &buf,
             int timeout_ms = -1) override;

        IOResult
        write(util::ByteBuffer &buf,
              int timeout_ms = -1) override;

        bool has_pending_output() const noexcept override;
        util::ResultV<void> flush() override;

    public:
        util::ByteBuffer &in_buffer() noexcept { return m_in; }
        util::ByteBuffer &out_buffer() noexcept { return m_out; }
        platform::net::UDPSocket &socket() noexcept { return m_sock; }
        const platform::net::UDPSocket &socket() const noexcept { return m_sock; }
    };
}

#endif // INCLUDE_EUNET_NET_CONNECTION_UDP_CONNECTION