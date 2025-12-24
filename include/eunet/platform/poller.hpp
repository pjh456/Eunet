#ifndef INCLUDE_EUNET_PLATFORM_POLLER
#define INCLUDE_EUNET_PLATFORM_POLLER

#include <cstdint>
#include <vector>

#include "eunet/platform/fd.hpp"

namespace platform::poller
{
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
        Poller();

        Poller(const Poller &) = delete;
        Poller &operator=(const Poller &) = delete;

        Poller(Poller &&) noexcept = default;
        Poller &operator=(Poller &&) noexcept = default;

    public:
        bool valid() const noexcept;

    public:
        bool add(int fd, std::uint32_t events) noexcept;
        bool modify(int fd, std::uint32_t events) noexcept;
        void remove(int fd) noexcept;

    public:
        std::vector<PollEvent> wait(int timeout_ms);
    };
}

#endif // INCLUDE_EUNET_PLATFORM_POLLER