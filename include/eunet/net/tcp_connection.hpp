#ifndef INCLUDE_EUNET_NET_TCP_CONNECTION
#define INCLUDE_EUNET_NET_TCP_CONNECTION

#include "eunet/platform/time.hpp"
#include "eunet/platform/tcp_socket.hpp"
#include "eunet/net/io_buffer.hpp"

namespace net::tcp
{
    class TCPConnection
    {
    private:
        platform::net::TCPSocket sock;
        IOBuffer m_in_buffer;  // 输入缓冲区
        IOBuffer m_out_buffer; // 输出缓冲区

    public:
        explicit TCPConnection(platform::net::TCPSocket sock);

        ~TCPConnection() = default;

    public:
        platform::net::TCPSocket &socket() { return sock; }
        IOBuffer &in_buffer() { return m_in_buffer; }
        IOBuffer &out_buffer() { return m_out_buffer; }

        bool has_pending_output() const { return m_out_buffer.readable_bytes() > 0; }
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