#include "eunet/platform/base_socket.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <utility>

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

    BaseSocket::BaseSocket(fd::Fd &&fd)
        : m_fd(std::move(fd)) {}

    fd::FdView
    BaseSocket::view()
        const noexcept { return m_fd.view(); }

    bool BaseSocket::is_open()
        const noexcept { return (bool)m_fd.view(); }

    void BaseSocket::close() noexcept { m_fd.reset(-1); }

    util::ResultV<Endpoint>
    BaseSocket::local_endpoint() const
    {
        using Result = util::ResultV<Endpoint>;
        using util::Error;

        sockaddr_storage addr{};
        socklen_t len = sizeof(addr);

        if (::getsockname(
                view().fd,
                reinterpret_cast<sockaddr *>(&addr),
                &len) < 0)
        {
            int err_no = errno;
            return Result::Err(
                Error::system()
                    .code(err_no)
                    .message("getsockname failed")
                    .build());
        }

        return Result::Ok(
            Endpoint(
                reinterpret_cast<sockaddr *>(&addr),
                static_cast<socklen_t>(len)));
    }

    util::ResultV<Endpoint>
    BaseSocket::remote_endpoint() const
    {
        using Result = util::ResultV<Endpoint>;
        using util::Error;

        sockaddr_storage addr{};
        socklen_t len = sizeof(addr);

        if (::getpeername(
                view().fd,
                reinterpret_cast<sockaddr *>(&addr),
                &len) < 0)
        {
            int err_no = errno;
            return Result::Err(
                Error::system()
                    .code(err_no)
                    .message("getpeername failed")
                    .build());
        }

        return Result::Ok(
            Endpoint(
                reinterpret_cast<sockaddr *>(&addr),
                static_cast<socklen_t>(len)));
    }
}
