#include "eunet/platform/fd.h"

#include <unistd.h>
#include <cerrno>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>

#include <utility>

namespace platform::fd
{
    Fd::Fd(int fd) noexcept : fd(fd) {}

    Fd::~Fd() noexcept { reset(-1); }

    Fd::Fd(Fd &&other) noexcept : fd(other.fd) { other.reset(-1); }

    Fd &Fd::operator=(Fd &&other) noexcept
    {
        if (this == &other)
            return *this;
        reset(-1);
        fd = other.fd;
        other.reset(-1);
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

    Fd Fd::socket(
        int domain,
        int type,
        int protocol) noexcept
    {
        int fd = ::socket(domain, type | SOCK_CLOEXEC, protocol);
        return Fd(fd);
    }

    std::tuple<Fd, Fd> Fd::pipe() noexcept
    {
        int fds[2];
        Fd read_end, write_end;

        if (::pipe2(fds, O_CLOEXEC) == 0)
        {
            read_end.reset(fds[0]);
            write_end.reset(fds[1]);
        }

        return {std::move(read_end), std::move(write_end)};
    }
}