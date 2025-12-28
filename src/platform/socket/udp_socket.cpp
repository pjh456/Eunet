#include "eunet/platform/socket/udp_socket.hpp"

#include <sys/socket.h>
#include <cerrno>

namespace platform::net
{
    util::ResultV<UDPSocket>
    UDPSocket::create(AddressFamily af)
    {
        using Result = util::ResultV<UDPSocket>;

        int domain =
            (af == AddressFamily::IPv6)
                ? AF_INET6
                : AF_INET;

        auto fd_res =
            fd::Fd::socket(
                domain,
                SOCK_DGRAM,
                0);

        if (fd_res.is_err())
            return util::ResultV<UDPSocket>::Err(fd_res.unwrap_err());

        return util::ResultV<UDPSocket>::Ok(
            UDPSocket(
                std::move(fd_res.unwrap())));
    }

    UDPSocket::UDPSocket(fd::Fd &&fd) noexcept
        : SocketBase(std::move(fd)) {}

    util::ResultV<void>
    UDPSocket::connect(
        const SocketAddress &peer)
    {
        using Result = util::ResultV<void>;

        if (::connect(
                view().fd,
                peer.as_sockaddr(),
                peer.length()) < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "UDP connect failed"));

        return Result::Ok();
    }

    util::ResultV<size_t>
    UDPSocket::send(
        const std::byte *data,
        size_t len)
    {
        using Result = util::ResultV<size_t>;

        ssize_t n = ::send(view().fd, data, len, 0);
        if (n < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "UDP send failed"));

        return Result::Ok(static_cast<size_t>(n));
    }

    util::ResultV<size_t>
    UDPSocket::recv(
        std::byte *buf,
        size_t len)
    {
        using Result = util::ResultV<size_t>;

        ssize_t n = ::recv(view().fd, buf, len, 0);
        if (n < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "UDP recv failed"));

        return Result::Ok(static_cast<size_t>(n));
    }
}
