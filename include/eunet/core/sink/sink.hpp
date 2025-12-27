#ifndef INCLUDE_EUNET_CORE_SINK_SINK
#define INCLUDE_EUNET_CORE_SINK_SINK

#include "eunet/core/event_snap_shoot.hpp"

namespace core::sink
{
    class IEventSink
    {
    public:
        virtual ~IEventSink() = default;

        virtual void on_event(const EventSnapshot &snap) = 0;
    };

}

#endif // INCLUDE_EUNET_CORE_SINK_SINK