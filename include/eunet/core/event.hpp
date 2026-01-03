#ifndef INCLUDE_EUNET_CORE_EVENT
#define INCLUDE_EUNET_CORE_EVENT

#include <string>
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

    struct Event
    {
    public:
        EventType type;
        platform::time::WallPoint ts;
        platform::fd::FdView fd{-1};
        SessionId session_id{0};

        std::string msg;
        util::Error error;

    public:
        static Event info(
            EventType type,
            std::string message,
            platform::fd::FdView fd = {-1}) noexcept;

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