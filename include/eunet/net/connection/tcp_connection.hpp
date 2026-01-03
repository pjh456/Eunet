#ifndef INCLUDE_EUNET_NET_CONNECTION_TCP_CONNECTION
#define INCLUDE_EUNET_NET_CONNECTION_TCP_CONNECTION

#include "eunet/util/byte_buffer.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/net/endpoint.hpp"
#include "eunet/platform/socket/tcp_socket.hpp"
#include "eunet/net/connection.hpp"

namespace net::tcp
{
    class TCPConnection;

    class TCPConnection final : public Connection
    {
    private:
        platform::net::TCPSocket m_sock;
        util::ByteBuffer m_in;
        util::ByteBuffer m_out;

    public:
        static util::ResultV<TCPConnection>
        connect(const platform::net::Endpoint &ep,
                platform::poller::Poller &poller,
                int timeout_ms = -1);

        static TCPConnection
        from_accepted_socket(platform::net::TCPSocket &&sock);

    public:
        explicit TCPConnection(platform::net::TCPSocket &&sock) noexcept;

        TCPConnection(const TCPConnection &) = delete;
        TCPConnection &operator=(const TCPConnection &) = delete;

        TCPConnection(TCPConnection &&) noexcept = default;
        TCPConnection &operator=(TCPConnection &&) noexcept = default;

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
        platform::net::TCPSocket &socket() noexcept { return m_sock; }
        const platform::net::TCPSocket &socket() const noexcept { return m_sock; }
    };
}

#endif // INCLUDE_EUNET_NET_CONNECTION_TCP_CONNECTION