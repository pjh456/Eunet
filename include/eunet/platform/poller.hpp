#ifndef INCLUDE_EUNET_PLATFORM_POLLER
#define INCLUDE_EUNET_PLATFORM_POLLER

#include <cstdint>
#include <vector>
#include <string>

#include "eunet/util/result.hpp"
#include "eunet/platform/fd.hpp"

namespace platform::poller
{

    enum class PollerErrorCode
    {
        NotInitialized,
        InvalidFd,
        AlreadyExists,
        NotFound,
        PermissionDenied,
        SystemError,
    };

    struct PollerError
    {
        PollerErrorCode code;
        int sys_errno;
        std::string message;
        std::string hint;
    };

    struct PollEvent
    {
        int fd;
        std::uint32_t events;
    };

    class Poller
    {
    private:
        static constexpr int MAX_EVENTS = 64;

    private:
        platform::fd::Fd epoll_fd;

    public:
        static util::Result<Poller, PollerError> create();

    private:
        Poller();

    public:
        Poller(const Poller &) = delete;
        Poller &operator=(const Poller &) = delete;

        Poller(Poller &&other) noexcept;
        Poller &operator=(Poller &&other) noexcept;

    public:
        bool valid() const noexcept;

        platform::fd::Fd &get_fd() noexcept;
        const platform::fd::Fd &get_fd() const noexcept;

    public:
        util::Result<bool, PollerError>
        add(int fd, std::uint32_t events) noexcept;
        util::Result<bool, PollerError>
        modify(int fd, std::uint32_t events) noexcept;
        util::Result<int, PollerError>
        remove(int fd) noexcept;

    public:
        util::Result<std::vector<PollEvent>, PollerError>
        wait(int timeout_ms);
    };
}

#endif // INCLUDE_EUNET_PLATFORM_POLLER