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

    bool Fd::operator!() const noexcept { return !valid(); }

    Fd::FdResult Fd::socket(
        int domain,
        int type,
        int protocol) noexcept
    {
        int fd = ::socket(domain, type | SOCK_CLOEXEC, protocol);
        if (fd < 0)
        {
            int err_no = errno;
            return FdResult::Err(
                util::Error::system()
                    .set_category(socket_errno_category(err_no))
                    .code(err_no)
                    .message("Failed to create socket")
                    .context("socket")
                    .build());
        }

        return FdResult::Ok(Fd(fd));
    }

    FdView Fd::view() const noexcept { return FdView{get()}; }

    Fd::PipeResult Fd::pipe() noexcept
    {
        using namespace util;
        int fds[2];

        if (::pipe2(fds, O_CLOEXEC) != 0)
        {
            int err_no = errno;
            return PipeResult::Err(
                util::Error::system()
                    .set_category(pipe_errno_category(err_no))
                    .code(err_no)
                    .message("Failed to create pipe")
                    .context("pipe2")
                    .build());
        }

        Pipe pipe;
        pipe.read.reset(fds[0]);
        pipe.write.reset(fds[1]);

        return PipeResult::Ok(std::move(pipe));
    }

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

    util::ErrorCategory
    Fd::socket_errno_category(int err)
    {
        using util::ErrorCategory;

        switch (err)
        {
        case EMFILE:
        case ENFILE:
        case ENOMEM:
            return ErrorCategory::ResourceExhausted;

        case EACCES:
            return ErrorCategory::AccessDenied;

        case EAFNOSUPPORT:
        case EPROTONOSUPPORT:
        case EINVAL:
            return ErrorCategory::InvalidInput;

        default:
            return ErrorCategory::Unknown;
        }
    }

    util::ErrorCategory
    Fd::pipe_errno_category(int err)
    {
        using util::ErrorCategory;
        switch (errno)
        {
        case EMFILE:
        case ENFILE:
            return ErrorCategory::ResourceExhausted;
        case EINVAL:
        case EFAULT:
            return ErrorCategory::InvalidInput;
        default:
            return ErrorCategory::Unknown;
        }
    }

    FdView FdView::from_owner(const Fd &owner)
    {
        return FdView{owner.get()};
    }

    FdView::operator bool() const noexcept { return fd >= 0; }

    bool FdView::operator==(const FdView &other) const noexcept { return (*this) && other && fd == other.fd; }

}

std::ostream &operator<<(
    std::ostream &os,
    const platform::fd::FdView fd)
{
    os << fd.fd;
    return os;
}