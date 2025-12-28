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
#include "eunet/platform/time.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/platform/sys_error.hpp"
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
    public:
    private:
        platform::fd::Fd sock;
        core::Orchestrator &orch;
        Options opts;

    public:
        explicit SyncTCPClient(core::Orchestrator &orch, Options opts = {});
        ~SyncTCPClient() noexcept;

        // 阶段化接口
        util::Result<void, platform::SysError> connect(const std::string &host, uint16_t port);
        util::Result<size_t, platform::SysError> send(const std::vector<std::byte> &data);
        util::Result<size_t, platform::SysError> recv(std::vector<std::byte> &buffer, size_t size);

        void close() noexcept;

    private:
        util::Result<void, platform::SysError>
        resolve_host(const std::string &host, sockaddr_in &out_addr);
        util::Result<void, core::EventError>
        emit_event(const core::Event &event);
    };
}

#endif // INCLUDE_EUNET_NET_SYNC_TCP_CLIENT