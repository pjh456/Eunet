#include "eunet/core/event.hpp"

#include <sstream>

namespace core
{

    Event Event::info(
        EventType type,
        std::string message,
        platform::fd::FdView fd) noexcept
    {
        Event e;
        e.type = type;
        e.fd = fd;

        e.msg = message;
        return e;
    }

    Event Event::failure(
        EventType type,
        util::Error error,
        platform::fd::FdView fd) noexcept
    {
        Event e;
        e.type = type;
        e.fd = fd;

        e.error = error;
        return e;
    }

    Event::Event() : ts(platform::time::wall_now()) {}

    bool Event::is_ok() const noexcept { return !error; }
    bool Event::is_error() const noexcept { return (bool)error; }

}

std::string to_string(const core::Event &event)
{
    std::stringstream ss;
    ss << " [" << static_cast<int>(event.type) << "] ";

    if (event.is_ok())
        ss << event.msg;
    else
    {
        ss << "ERROR[";
        if (event.error)
            ss << to_string(event.error.get_domain());
        ss << "]: " << event.error.get_message();
    }

    if (event.fd)
        ss << " fd=" << event.fd;
    return ss.str();
}