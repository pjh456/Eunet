#ifndef INCLUDE_EUNET_CORE_EVENT
#define INCLUDE_EUNET_CORE_EVENT

#include <string>
#include <optional>
#include <variant>
#include <any>

#include "eunet/util/result.hpp"
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
        // Lifecycle
        CONNECTION_IDLE, // 连接闲置中
        CONNECTION_CLOSED
    };

    struct EventError
    {
        std::string domain;  // 来源模块
        std::string message; // 已 format 的错误
    };

    struct Event
    {
    public:
        using EventData = std::any;
        // using EventData = std::variant<
        //     std::monostate>;
        // DnsResult,
        // TcpInfo,
        // TlsCipherSuite,
        // HttpHeaderMap>;

    public:
        EventType type;
        platform::time::WallPoint ts;
        int fd{-1};

        std::string msg;
        std::optional<EventError> error{std::nullopt};
        EventData data{};

    public:
        static Event info(
            EventType type,
            std::string message,
            int fd = -1,
            EventData data = {}) noexcept;

        static Event failure(
            EventType type,
            EventError error,
            int fd = -1,
            EventData data = {}) noexcept;

    private:
        Event();

    public:
        ~Event() = default;

    public:
        bool is_ok() const noexcept;
        bool is_error() const noexcept;

        std::string to_string() const;
    };
}

#endif // INCLUDE_EUNET_CORE_EVENT