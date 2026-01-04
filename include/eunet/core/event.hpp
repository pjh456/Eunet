/*
 * ============================================================================
 *  File Name   : event.hpp
 *  Module      : core
 *
 *  Description :
 *      系统核心事件定义。包含事件类型 (EventType)、关联的 FD、时间戳
 *      以及可选的错误信息。是系统各模块间通信的基本单元。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_CORE_EVENT
#define INCLUDE_EUNET_CORE_EVENT

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <any>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/fd.hpp"

namespace core
{
    enum class EventType
    {
        // DNS
        DNS_RESOLVE_START,
        DNS_RESOLVE_DONE,
        // TCP
        TCP_CONNECT_START,
        TCP_CONNECT_SUCCESS,
        TCP_CONNECT_TIMEOUT,
        // TLS
        TLS_HANDSHAKE_START,
        TLS_HANDSHAKE_DONE,
        // Data
        HTTP_SENT,
        HTTP_RECEIVED,
        HTTP_REQUEST_BUILD,
        HTTP_HEADERS_RECEIVED,
        HTTP_BODY_DONE,
        // Lifecycle
        CONNECTION_IDLE, // 连接闲置中
        CONNECTION_CLOSED
    };

    using SessionId = uint64_t;

    /**
     * @brief 系统事件定义
     *
     * 代表系统中发生的任何有意义的状态变更。
     * 事件是不可变的（创建后），并携带了发生的时间戳、关联的 FD 和 Session。
     */
    struct Event
    {
    public:
        /** 事件类型枚举 */
        EventType type;
        /** 事件发生的墙上时间 */
        platform::time::WallPoint ts;
        /** 关联的文件描述符（如果有） */
        platform::fd::FdView fd{-1};
        /** 关联的会话 ID */
        SessionId session_id{0};

        std::string msg;
        std::optional<util::Error> error = std::nullopt;
        std::optional<std::vector<std::byte>> payload = std::nullopt;

    public:
        static Event info(
            EventType type,
            std::string message,
            platform::fd::FdView fd = {-1},
            std::optional<std::vector<std::byte>> payload = std::nullopt) noexcept;

        static Event failure(
            EventType type,
            util::Error err,
            platform::fd::FdView fd = {-1}) noexcept;

    private:
        Event();

    public:
        ~Event() = default;

    public:
        bool is_ok() const noexcept;
        bool is_error() const noexcept;
    };
}

std::string to_string(core::EventType type);
std::string to_string(const core::Event &event);

#endif // INCLUDE_EUNET_CORE_EVENT