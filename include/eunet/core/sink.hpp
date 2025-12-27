#ifndef INCLUDE_EUNET_CORE_SINK
#define INCLUDE_EUNET_CORE_SINK

#include "eunet/core/event_snapshot.hpp"

namespace core::sink
{
    class IEventSink
    {
    public:
        virtual ~IEventSink() = default;

        virtual void on_event(const EventSnapshot &snap) = 0;
    };

}

#endif // INCLUDE_EUNET_CORE_SINK