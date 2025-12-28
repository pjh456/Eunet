#ifndef INCLUDE_EUNET_NET_SYNC_TCP_CLIENT
#define INCLUDE_EUNET_NET_SYNC_TCP_CLIENT

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/core/timeline.hpp"
#include "eunet/core/orchestrator.hpp"

namespace net::tcp
{
    struct Options
    {
        std::chrono::milliseconds timeout{5000};
    };

    class SyncTCPClient
    {
    private:
        platform::fd::Fd sock;
        core::Orchestrator &orch;
        Options opts;

    public:
        explicit SyncTCPClient(core::Orchestrator &orch, Options opts = {});
        ~SyncTCPClient() noexcept;

        // 阶段化接口
        util::ResultV<void> connect(const std::string &host, uint16_t port);
        util::ResultV<size_t> send(const std::vector<std::byte> &data);

        util::ResultV<size_t> recv(std::vector<std::byte> &buffer, size_t size);

        void close() noexcept;

    private:
        util::ResultV<void> resolve_host(const std::string &host, sockaddr_in &out_addr);

        util::ResultV<void> emit_event(const core::Event &event);
    };
}

#endif // INCLUDE_EUNET_NET_SYNC_TCP_CLIENT