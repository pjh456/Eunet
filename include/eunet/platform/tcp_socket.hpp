#ifndef INCLUDE_EUNET_PLATFORM_TCP_SOCKET
#define INCLUDE_EUNET_PLATFORM_TCP_SOCKET

#include "eunet/util/error.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/platform/address.hpp"

namespace platform::net
{
    class TCPSocket
    {
    private:
        fd::Fd fd;

    public:
        static util::ResultV<TCPSocket> create();

    public:
        explicit TCPSocket(fd::Fd fd);

    public:
        util::ResultV<void>
        connect(
            const SocketAddress &addr,
            time::Duration timeout);

    public:
        util::ResultV<size_t>
        send(
            const std::byte *data,
            size_t len,
            time::Duration timeout);
        util::ResultV<size_t>
        recv(
            std::byte *buf,
            size_t len,
            time::Duration timeout);

    public:
        void set_nonblocking(bool enable);

        fd::FdView view() const;

        void close();
    };
}
#endif