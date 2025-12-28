#include "eunet/platform/socket_base.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace platform::net
{
    SocketBase::SocketBase(fd::Fd fd)
        : m_fd(std::move(fd)) {}

    fd::FdView
    SocketBase::view()
        const noexcept { return m_fd.view(); }

    void
    SocketBase::set_nonblocking(
        bool enable) noexcept
    {
        int flags = fcntl(m_fd.view().fd, F_GETFL, 0);
        if (flags < 0)
            return;

        if (enable)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;

        fcntl(m_fd.view().fd, F_SETFL, flags);
    }

    void SocketBase::close() noexcept { m_fd.reset(-1); }
}
