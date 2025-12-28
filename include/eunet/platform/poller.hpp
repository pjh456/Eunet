#ifndef INCLUDE_EUNET_PLATFORM_POLLER
#define INCLUDE_EUNET_PLATFORM_POLLER

#include <cstdint>
#include <vector>
#include <string>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/platform/fd.hpp"

namespace platform::poller
{
    struct PollEvent
    {
        platform::fd::FdView fd;
        std::uint32_t events;
    };

    class Poller
    {

    private:
        static constexpr int MAX_EVENTS = 64;

    private:
        platform::fd::Fd epoll_fd;

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

    public:
        util::ResultV<void> add(
            const platform::fd::Fd &fd,
            std::uint32_t events) noexcept;
        util::ResultV<void> modify(
            const platform::fd::Fd &fd,
            std::uint32_t events) noexcept;
        util::ResultV<void>
        remove(const platform::fd::Fd &fd) noexcept;

    public:
        util::ResultV<std::vector<PollEvent>>
        wait(int timeout_ms) noexcept;
    };
}

#endif // INCLUDE_EUNET_PLATFORM_POLLER