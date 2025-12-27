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
        util::Result<bool, PollerError>
        add(int fd, std::uint32_t events) noexcept;
        util::Result<bool, PollerError>
        modify(int fd, std::uint32_t events) noexcept;
        util::Result<bool, PollerError>
        remove(int fd) noexcept;

    public:
        util::Result<std::vector<PollEvent>, SysError>
        wait(int timeout_ms) noexcept;
    };
}

const char *to_string(const platform::poller::PollerErrorCode &code);

std::string format_error(const platform::poller::PollerError &e);

#endif // INCLUDE_EUNET_PLATFORM_POLLER