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
}
