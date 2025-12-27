#include "eunet/core/event.hpp"

#include <sstream>

namespace core
{

    Event Event::info(
        EventType type,
        std::string message,
        int fd,
        std::any data) noexcept
    {
        Event e;
        e.type = type;
        e.fd = fd;

        e.msg = message;
        e.data = data;
        return e;
    }

    Event Event::failure(
        EventType type,
        EventError error,
        int fd,
        std::any data) noexcept
    {
        Event e;
        e.type = type;
        e.fd = fd;

        e.error = error;
        e.data = data;
        return e;
    }

    Event::Event() : ts(platform::time::wall_now()) {}

    bool Event::is_ok() const noexcept { return !error.has_value(); }
    bool Event::is_error() const noexcept { return error.has_value(); }

    std::string Event::to_string() const
    {
        std::stringstream ss;
        ss << " [" << static_cast<int>(type) << "] ";

        if (is_ok())
            ss << msg;
        else
            ss << "ERROR[" << error->domain << "]: " << error->message;

        if (fd != -1)
            ss << " fd=" << fd;
        return ss.str();
    }
}