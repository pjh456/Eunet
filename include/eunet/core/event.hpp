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
        ERROR
    };

    struct Error
    {
        int code = 0;
        std::string message;
    };

    struct Event
    {
    public:
        using MsgResult = util::Result<std::string, Error>;

    public:
        EventType type;
        platform::time::WallPoint ts;
        MsgResult msg;
        int fd;
        std::any data;

    public:
        static Event create(
            EventType type,
            util::Result<std::string, Error>
                msg = MsgResult::Ok(""),
            int fd = -1,
            std::any data = {});

    private:
        Event();

    public:
        ~Event() = default;

    public:
        std::string to_string() const;
    };
}

#endif // INCLUDE_EUNET_CORE_EVENT