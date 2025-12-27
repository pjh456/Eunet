#include "eunet/platform/fd.hpp"

#include <unistd.h>
#include <cerrno>
#include <sys/socket.h>
#include <fcntl.h>

namespace platform::fd
{
    Fd::Fd(int fd) noexcept : fd(fd) {}

    Fd::~Fd() noexcept
    {
        if (fd >= 0)
            while (::close(fd) && errno == EINTR)
                ;
    }

    Fd::Fd(Fd &&other) noexcept : fd(other.fd) { other.release(); }

    Fd &Fd::operator=(Fd &&other) noexcept
    {
        if (this == &other)
            return *this;
        reset(-1);
        fd = other.fd;
        other.release();
        return *this;
    }

    int Fd::get() const noexcept { return fd; }

    bool Fd::valid() const noexcept { return fd >= 0; }

    Fd::operator bool() const noexcept { return valid(); }

    int Fd::release() noexcept
    {
        int old = fd;
        fd = -1;
        return old;
    }

    void Fd::reset(int new_fd) noexcept
    {
        if (fd >= 0)
        {
            while (::close(fd) && errno == EINTR)
                ;
        }
        fd = new_fd;
    }

    SysResult<Fd> Fd::socket(
        int domain,
        int type,
        int protocol) noexcept
    {
        int fd = ::socket(domain, type | SOCK_CLOEXEC, protocol);
        if (fd < 0)
            return SysResult<Fd>::Err(SysError::from_errno(errno));

        return SysResult<Fd>::Ok(Fd(fd));
    }

    SysResult<Pipe> Fd::pipe() noexcept
    {
        int fds[2];
        Pipe pipe;

        if (::pipe2(fds, O_CLOEXEC) != 0)
            return SysResult<Pipe>::Err(SysError::from_errno(errno));

        pipe.read.reset(fds[0]);
        pipe.write.reset(fds[1]);

        return SysResult<Pipe>::Ok(std::move(pipe));
    }
}