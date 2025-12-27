#ifndef INCLUDE_EUNET_CORE_EVENT
#define INCLUDE_EUNET_CORE_EVENT

#include <string>
#include <optional>
#include <any>

#include "eunet/util/result.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/platform/fd.hpp"

namespace core
{
    enum class EventType
    {
        DNS_START,
        DNS_DONE,
        TCP_CONNECT,
        TCP_ESTABLISHED,
        REQUEST_SENT,
        REQUEST_RECEIVED,
        CLOSED,
    };

    struct EventError
    {
        std::string domain;  // 来源模块
        std::string message; // 已 format 的错误
    };

    struct Event
    {
    public:
        EventType type;
        platform::time::WallPoint ts;
        int fd{-1};

        std::string msg;
        std::optional<EventError> error{std::nullopt};
        std::any data{};

    public:
        static Event info(
            EventType type,
            std::string message,
            int fd = -1,
            std::any data = {}) noexcept;

        static Event failure(
            EventType type,
            EventError error,
            int fd = -1,
            std::any data = {}) noexcept;

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