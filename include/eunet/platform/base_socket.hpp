#ifndef INCLUDE_EUNET_PLATFORM_BASE_SOCKET
#define INCLUDE_EUNET_PLATFORM_BASE_SOCKET

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/util/byte_buffer.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/platform/poller.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/net/endpoint.hpp"

namespace platform::net
{
    using IOResult = util::ResultV<size_t>;
    class BaseSocket
    {
    protected:
        fd::Fd m_fd;
        poller::Poller &m_poller;

        explicit BaseSocket(fd::Fd &&fd, poller::Poller &poller);

    public:
        BaseSocket(const BaseSocket &) = delete;
        BaseSocket &operator=(const BaseSocket &) = delete;

        BaseSocket(BaseSocket &&) noexcept = default;
        BaseSocket &operator=(BaseSocket &&) noexcept = default;

        ~BaseSocket() = default;

    public:
        fd::FdView view() const noexcept;
        bool is_open() const noexcept;
        void close() noexcept;

        util::ResultV<Endpoint>
        local_endpoint() const;

        util::ResultV<Endpoint>
        remote_endpoint() const;

    public:
        virtual IOResult
        read(util::ByteBuffer &buf, int timeout_ms = -1) = 0;

        virtual IOResult
        write(util::ByteBuffer &buf, int timeout_ms = -1) = 0;

        virtual util::ResultV<void>
        connect(const Endpoint &ep, int timeout_ms = -1) = 0;
    };

    util::ResultV<void>
    wait_fd_epoll(
        poller::Poller &poller,
        fd::FdView fd,
        uint32_t events,
        int timeout_ms = -1);
}

#endif // INCLUDE_EUNET_PLATFORM_BASE_SOCKET