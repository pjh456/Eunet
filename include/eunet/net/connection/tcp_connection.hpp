#ifndef INCLUDE_EUNET_NET_CONNECTION_TCP_CONNECTION
#define INCLUDE_EUNET_NET_CONNECTION_TCP_CONNECTION

#include "eunet/platform/time.hpp"
#include "eunet/platform/tcp_socket.hpp"
#include "eunet/net/io_buffer.hpp"
#include "eunet/net/connection.hpp"

namespace net::tcp
{
    class TCPConnection;

    class TCPConnection final : public Connection
    {
    private:
        platform::net::TCPSocket sock;
        IOBuffer m_in_buffer;
        IOBuffer m_out_buffer;

    public:
        static util::ResultV<TCPConnection>
        connect(
            const platform::net::SocketAddress &addr,
            platform::time::Duration timeout);

        static TCPConnection
        from_accepted_socket(
            platform::net::TCPSocket &&sock);

    private:
        explicit TCPConnection(
            platform::net::TCPSocket &&sock);

    public:
        TCPConnection(TCPConnection &&) noexcept = default;
        TCPConnection &operator=(TCPConnection &&) noexcept = default;

        ~TCPConnection() override = default;

    public:
        platform::fd::FdView fd() const noexcept override;
        void close() noexcept override;
        bool is_open() const noexcept override;

        util::ResultV<size_t>
        read(std::byte *buf, size_t len,
             platform::time::Duration timeout) override;

        util::ResultV<size_t>
        write(const std::byte *buf, size_t len,
              platform::time::Duration timeout) override;

        bool has_pending_output() const noexcept override;
        util::ResultV<void> flush() override;

    public:
        IOBuffer &in_buffer();
        IOBuffer &out_buffer();

        util::ResultV<size_t> read_available();
    };
}

#endif // INCLUDE_EUNET_NET_CONNECTION_TCP_CONNECTION