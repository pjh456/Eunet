#ifndef INCLUDE_EUNET_NET_TCP_CONNECTION
#define INCLUDE_EUNET_NET_TCP_CONNECTION

#include "eunet/platform/time.hpp"
#include "eunet/platform/tcp_socket.hpp"
#include "eunet/net/io_buffer.hpp"

namespace net::tcp
{
    class TCPConnection;

    class TCPConnection
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

        ~TCPConnection() = default;

    public:
        IOBuffer &in_buffer();
        IOBuffer &out_buffer();

        platform::fd::FdView fd() const noexcept;
        void set_nonblocking(bool enable);

        bool has_pending_output() const;
        void close();

    public:
        util::ResultV<void> send(
            const std::byte *data,
            size_t len,
            platform::time::Duration timeout);
        util::ResultV<void> send(
            const std::vector<std::byte> &data,
            platform::time::Duration timeout);

        util::ResultV<void> flush();

    public:
        util::ResultV<size_t> read_available();
    };
}

#endif // INCLUDE_EUNET_NET_TCP_CONNECTION