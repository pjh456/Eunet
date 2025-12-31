#ifndef INCLUDE_EUNET_NET_ASYNC_TCP_CLIENT
#define INCLUDE_EUNET_NET_ASYNC_TCP_CLIENT

#include <string>
#include <vector>
#include <netinet/in.h>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/platform/poller.hpp"
#include "eunet/core/orchestrator.hpp"

namespace net::tcp
{
    class AsyncTCPClient
    {
    private:
        core::Orchestrator &orch;
        platform::fd::Fd sock;

    public:
        explicit AsyncTCPClient(core::Orchestrator &orch);
        ~AsyncTCPClient() = default;

    public:
        /**
         * 非阻塞连接
         * 流程：DNS(同步) -> Socket(非阻塞) -> Connect(EINPROGRESS) -> Poller等待
         */
        util::ResultV<void> connect(const std::string &host, uint16_t port, int timeout_ms = 5000);

        util::ResultV<size_t> send(const std::vector<std::byte> &data);
        util::ResultV<size_t> recv(std::vector<std::byte> &buffer, size_t max_size);

        void close() noexcept;

    private:
        util::ResultV<void> set_nonblocking(int fd);
        util::ResultV<void> emit_event(const core::Event &event);
        util::ResultV<sockaddr_in> resolve_host(const std::string &host, uint16_t port);

    private:
        static util::ErrorCategory connect_errno_category(int err);
    };
}

#endif