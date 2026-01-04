/*
 * ============================================================================
 *  File Name   : tcp_client.hpp
 *  Module      : net/tcp
 *
 *  Description :
 *      高层 TCP 客户端封装。组合了 Poller、TCPConnection 和 DNS Resolver。
 *      管理连接建立的过程（DNS -> Connect），提供简化的 send/recv 接口，
 *      并负责生成 TCP 层的生命周期事件。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_NET_TCP_CLIENT
#define INCLUDE_EUNET_NET_TCP_CLIENT

#include <vector>
#include <string>
#include <cstddef>
#include <optional>

#include "eunet/core/orchestrator.hpp"
#include "eunet/util/result.hpp"
#include "eunet/net/connection/tcp_connection.hpp"

namespace net::tcp
{
    /**
     * @brief 可观测的 TCP 客户端
     *
     * 封装了 TCP 连接建立和数据收发过程。
     * 关键特性是它会在关键节点（DNS, Connect Start, Success, Send, Recv）
     * 主动向 Orchestrator 发送 Event，从而实现可视化。
     */
    class TCPClient
    {
    private:
        core::Orchestrator &orch;
        std::optional<TCPConnection> m_conn;
        platform::poller::Poller m_poller;

    public:
        explicit TCPClient(core::Orchestrator &o);
        ~TCPClient();

        // 禁止拷贝，允许移动
        TCPClient(const TCPClient &) = delete;
        TCPClient &operator=(const TCPClient &) = delete;
        TCPClient(TCPClient &&) = default;
        TCPClient &operator=(TCPClient &&) = default;

    public:
        /**
         * @brief 发起连接
         *
         * 执行 DNS 解析（如果需要）并建立 TCP 连接。
         * 过程中会产生多个 Event。
         *
         * @param host 目标主机名或 IP
         * @param port 目标端口
         * @param timeout_ms 超时限制
         */
        util::ResultV<void> connect(
            const std::string &host, uint16_t port, int timeout_ms = 3000);
        util::ResultV<size_t> send(
            const std::vector<std::byte> &data, int timeout_ms = 3000);
        util::ResultV<size_t> recv(
            std::vector<std::byte> &buffer, size_t max_size, int timeout_ms = 3000);
        void close() noexcept;

    private:
        util::ResultV<void> emit_event(const core::Event &e);
    };
}

#endif // INCLUDE_EUNET_NET_TCP_CLIENT