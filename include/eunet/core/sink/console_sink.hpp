#ifndef INCLUDE_EUNET_CORE_SINK_CONSOLE_SINK
#define INCLUDE_EUNET_CORE_SINK_CONSOLE_SINK

#include <iostream>

#include "eunet/core/sink/sink.hpp"

namespace core::sink
{
    class ConsoleSink : public IEventSink
    {
    public:
        void on_event(const EventSnapshot &s) override
        {
            std::cout
                << "[fd=" << s.fd << "] "
                << to_string(s.state)
                << " @ " << to_string(s.ts)
                << "\n";
        }
    };
}

#endif // INCLUDE_EUNET_CORE_SINK_CONSOLE_SINK