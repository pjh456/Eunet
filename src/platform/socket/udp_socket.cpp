#include "eunet/platform/socket/udp_socket.hpp"

#include <sys/socket.h>
#include <cerrno>

namespace platform::net
{
    util::ResultV<UDPSocket>
    UDPSocket::create()
    {
        using Result = util::ResultV<UDPSocket>;

        int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0)
            return Result::Err(
                util::Error::from_errno(errno, "UDP socket create failed"));

        return Result::Ok(UDPSocket(fd::Fd(fd)));
    }

    UDPSocket::UDPSocket(fd::Fd fd)
        : SocketBase(std::move(fd)) {}

    util::ResultV<void>
    UDPSocket::connect(const SocketAddress &peer)
    {
        using Result = util::ResultV<void>;

        if (::connect(
                m_fd.view().fd,
                peer.as_sockaddr(),
                peer.length()) < 0)
            return Result::Err(util::Error::from_errno(errno, "UDP connect failed"));

        return Result::Ok();
    }

    util::ResultV<size_t>
    UDPSocket::send(
        const std::byte *data,
        size_t len,
        time::Duration)
    {
        using Result = util::ResultV<size_t>;

        ssize_t n = ::send(
            m_fd.view().fd, data, len, 0);

        if (n < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "UDP send failed"));

        return Result::Ok(static_cast<size_t>(n));
    }

    util::ResultV<size_t>
    UDPSocket::recv(
        std::byte *buf,
        size_t len,
        time::Duration)
    {
        using Result = util::ResultV<size_t>;

        ssize_t n = ::recv(
            m_fd.view().fd, buf, len, 0);

        if (n < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "UDP recv failed"));

        return Result::Ok(static_cast<size_t>(n));
    }
}
