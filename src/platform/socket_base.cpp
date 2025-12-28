#include "eunet/platform/socket_base.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace platform::net
{
    NonBlockingGuard::NonBlockingGuard(fd::FdView fd) noexcept
        : m_fd(fd), m_old_flags(-1)
    {
        if (!m_fd)
            return;

        m_old_flags = ::fcntl(m_fd.fd, F_GETFL, 0);
        if (m_old_flags >= 0)
            ::fcntl(m_fd.fd, F_SETFL, m_old_flags | O_NONBLOCK);
    }

    NonBlockingGuard::~NonBlockingGuard() noexcept
    {
        if (m_fd && m_old_flags >= 0)
            ::fcntl(m_fd.fd, F_SETFL, m_old_flags);
    }

    SocketBase::SocketBase(fd::Fd fd)
        : m_fd(std::move(fd)) {}

    fd::FdView
    SocketBase::view()
        const noexcept { return m_fd.view(); }

    bool SocketBase::is_open()
        const noexcept { return (bool)m_fd.view(); }

    void SocketBase::close() noexcept { m_fd.reset(-1); }

    util::ResultV<SocketAddress>
    SocketBase::local_address() const
    {
        using Result = util::ResultV<SocketAddress>;

        sockaddr_storage addr{};
        socklen_t len = sizeof(addr);

        if (::getsockname(
                view().fd,
                reinterpret_cast<sockaddr *>(&addr),
                &len) < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "getsockname failed"));

        return Result::Ok(
            SocketAddress(
                reinterpret_cast<sockaddr *>(&addr), len));
    }

    util::ResultV<SocketAddress>
    SocketBase::peer_address() const
    {
        using Result = util::ResultV<SocketAddress>;

        sockaddr_storage addr{};
        socklen_t len = sizeof(addr);

        if (::getpeername(
                view().fd,
                reinterpret_cast<sockaddr *>(&addr),
                &len) < 0)
            return Result::Err(
                util::Error::from_errno(
                    errno, "getpeername failed"));

        return Result::Ok(
            SocketAddress(
                reinterpret_cast<sockaddr *>(&addr), len));
    }
}
