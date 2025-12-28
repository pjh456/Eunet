#ifndef INCLUDE_EUNET_PLATFORM_POLLER
#define INCLUDE_EUNET_PLATFORM_POLLER

#include <cstdint>
#include <vector>
#include <string>

#include "eunet/util/result.hpp"
#include "eunet/platform/sys_error.hpp"
#include "eunet/platform/fd.hpp"

namespace platform::poller
{

    enum class PollerErrorCode
    {
        NotInitialized,
        InvalidFd,
        AlreadyExists,
        NotFound,
    };

    struct PollerError
    {
        PollerErrorCode code;
        SysError cause;
    };

    struct PollEvent
    {
        platform::fd::FdView fd;
        std::uint32_t events;
    };

    class Poller
    {
    public:
        using PollerResult = util::Result<void, PollerError>;

    private:
        static constexpr int MAX_EVENTS = 64;

    private:
        platform::fd::Fd epoll_fd;

    public:
        static SysResult<Poller> create();

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
        PollerResult add(const platform::fd::Fd &fd, std::uint32_t events) noexcept;
        PollerResult modify(const platform::fd::Fd &fd, std::uint32_t events) noexcept;
        PollerResult remove(const platform::fd::Fd &fd) noexcept;

    public:
        SysResult<std::vector<PollEvent>>
        wait(int timeout_ms) noexcept;
    };
}

const char *to_string(const platform::poller::PollerErrorCode &code);

std::string format_error(const platform::poller::PollerError &e);

#endif // INCLUDE_EUNET_PLATFORM_POLLER