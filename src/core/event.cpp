#include "eunet/core/event.hpp"

#include <sstream>

namespace core
{
    Event Event::create(
        EventType type,
        util::Result<std::string, Error> msg,
        int fd, std::any data)
    {
        Event e;
        e.type = type;
        e.ts = platform::time::wall_now();
        e.msg = std::move(msg);
        e.fd = fd;
        e.data = std::move(data);
        return e;
    }

    Event::Event()
        : type(EventType::ERROR),
          msg(MsgResult::Err(Error{0, "Unknown Error"})) {}

    std::string Event::to_string() const
    {
        std::stringstream ss;
        ss << " [" << static_cast<int>(type) << "] ";
        if (msg.is_ok())
            ss << msg.unwrap();
        else
            ss << "ERROR(" << msg.unwrap_err().code << "): " << msg.unwrap_err().message;
        if (fd != -1)
            ss << " fd=" << fd;
        return ss.str();
    }
}