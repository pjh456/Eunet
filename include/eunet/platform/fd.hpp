#ifndef INCLUDE_EUNET_PLATFORM_FD
#define INCLUDE_EUNET_PLATFORM_FD

#include <ostream>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
// #include "eunet/platform/sys_error.hpp"

namespace platform::fd
{
    class Fd;
    struct FdView;
    struct Pipe;

    class Fd
    {
    public:
        using FdResult = util::ResultV<Fd>;
        using PipeResult = util::ResultV<Pipe>;

    private:
        int fd;

    public:
        explicit Fd(int fd = -1) noexcept;
        ~Fd() noexcept;

        Fd(const Fd &) = delete;
        Fd &operator=(const Fd &) = delete;

        Fd(Fd &&other) noexcept;
        Fd &operator=(Fd &&other) noexcept;

    public:
        int get() const noexcept;
        bool valid() const noexcept;
        explicit operator bool() const noexcept;
        bool operator!() const noexcept;

    public:
        static FdResult
        socket(
            int domain,
            int type,
            int protocol) noexcept;

        FdView view() const noexcept;

        static PipeResult pipe() noexcept;

    public:
        int release() noexcept;
        void reset(int new_fd) noexcept;

    private:
        static util::ErrorCategory socket_errno_category(int err);
        static util::ErrorCategory pipe_errno_category(int err);
    };

    struct FdView
    {
        int fd;
        static FdView from_owner(const Fd &owner);
        explicit operator bool() const noexcept;

        bool operator==(const FdView &other) const noexcept;
    };

    struct Pipe
    {
        Fd read;
        Fd write;
    };
}

std::ostream &operator<<(
    std::ostream &os,
    const platform::fd::FdView fd);

#endif // INCLUDE_EUNET_PLATFORM_FD