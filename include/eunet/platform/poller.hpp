#ifndef INCLUDE_EUNET_PLATFORM_POLLER
#define INCLUDE_EUNET_PLATFORM_POLLER

#include <sys/epoll.h>
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_set>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/platform/fd.hpp"

namespace platform::poller
{
    enum class PollEventType : uint32_t
    {
        Read = EPOLLIN,
        Write = EPOLLOUT,
        Error = EPOLLERR | EPOLLHUP,
    };

    struct PollEvent;

    struct PollEvent
    {
        platform::fd::FdView fd;
        std::uint32_t events;

    public:
        bool is_readable() { return events & static_cast<uint32_t>(PollEventType::Read); }
        bool is_writable() { return events & static_cast<uint32_t>(PollEventType::Write); }
    };

    class Poller
    {
    public:
        using FdTable = std::unordered_set<int>;

    private:
        static constexpr int MAX_EVENTS = 64;

    private:
        platform::fd::Fd epoll_fd;
        FdTable fd_table;

    public:
        static util::ResultV<Poller> create();

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

        bool has_fd(int fd) const noexcept;

    public:
        util::ResultV<void> add(
            platform::fd::FdView fd,
            std::uint32_t events) noexcept;
        util::ResultV<void> modify(
            platform::fd::FdView fd,
            std::uint32_t events) noexcept;
        util::ResultV<void>
        remove(platform::fd::FdView fd) noexcept;

    public:
        util::ResultV<std::vector<PollEvent>>
        wait(int timeout_ms) noexcept;

    private:
        static util::ErrorCategory
        epoll_errno_category(int err);
    };
}

#endif // INCLUDE_EUNET_PLATFORM_POLLER