#ifndef INCLUDE_EUNET_CORE_I_EVENT_SINK
#define INCLUDE_EUNET_CORE_I_EVENT_SINK

#include "eunet/core/event.hpp"
#include "eunet/core/lifecycle_fsm.hpp"

namespace core::sink
{
    class IEventSink
    {
    public:
        virtual ~IEventSink() = default;

        virtual void on_event(
            const Event &e,
            const LifecycleFSM &fsm) = 0;
    };

}

#endif // INCLUDE_EUNET_CORE_I_EVENT_SINK