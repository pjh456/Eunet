#ifndef INCLUDE_EUNET_NET_TCP_CLIENT
#define INCLUDE_EUNET_NET_TCP_CLIENT

#include <vector>
#include <string>
#include <cstddef>
#include <optional>

#include "eunet/core/orchestrator.hpp"
#include "eunet/util/result.hpp"
#include "eunet/platform/socket/tcp_socket.hpp"

namespace net::tcp
{
    class TCPClient
    {
    private:
        core::Orchestrator &orch;
        // 使用 optional 管理 socket 生命周期，替代原本裸露的 Fd
        std::optional<platform::net::TCPSocket> m_sock;

    public:
        explicit TCPClient(core::Orchestrator &o);
        ~TCPClient();

        // 禁止拷贝，允许移动
        TCPClient(const TCPClient &) = delete;
        TCPClient &operator=(const TCPClient &) = delete;
        TCPClient(TCPClient &&) = default;
        TCPClient &operator=(TCPClient &&) = default;

    public:
        util::ResultV<void> connect(
            const std::string &host, uint16_t port, int timeout_ms = 3000);
        util::ResultV<size_t> send(
            const std::vector<std::byte> &data);
        util::ResultV<size_t> recv(
            std::vector<std::byte> &buffer, size_t max_size);
        void close() noexcept;

    private:
        util::ResultV<void> emit_event(const core::Event &e);
    };
}

#endif // INCLUDE_EUNET_NET_TCP_CLIENT