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

std::string to_string(core::EventType type)
{
    using core::EventType;
    switch (type)
    {
    case EventType::DNS_RESOLVE_START:
        return "DNS Resolve Start";
    case EventType::DNS_RESOLVE_DONE:
        return "DNS Resolve Done";

    case EventType::TCP_CONNECT_START:
        return "TCP Connection Start";
    case EventType::TCP_CONNECT_SUCCESS:
        return "TCP Connection Success";
    case EventType::TCP_CONNECT_TIMEOUT:
        return "TCP Connection Timeout";

    case EventType::TLS_HANDSHAKE_START:
        return "TLS Handshake Start";
    case EventType::TLS_HANDSHAKE_DONE:
        return "TLS Handshake Done";

    case EventType::HTTP_SENT:
        return "HTTP Sent";
    case EventType::HTTP_RECEIVED:
        return "HTTP Received";
    case EventType::HTTP_REQUEST_BUILD:
        return "HTTP Request Build";
    case EventType::HTTP_HEADERS_RECEIVED:
        return "HTTP Headers Received";
    case EventType::HTTP_BODY_DONE:
        return "HTTP Body Done";

    case EventType::CONNECTION_IDLE:
        return "Connection Idle";
    case EventType::CONNECTION_CLOSED:
        return "Connection Closed";
    default:
        return "Unknown";
    }
}

std::string to_string(const core::Event &event)
{
    std::stringstream ss;
    ss << "[" << to_string(event.type) << "] ";

    if (event.is_ok())
        ss << event.msg;
    else
    {
        ss << "ERROR[";
        if (event.error)
            ss << to_string(event.error->domain());
        ss << "]: " << event.error->message();
    }

    if (event.fd)
        ss << " fd=" << event.fd;
    return ss.str();
}