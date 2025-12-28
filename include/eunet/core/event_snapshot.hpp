#ifndef INCLUDE_EUNET_CORE_EVENT_SNAPSHOT
#define INCLUDE_EUNET_CORE_EVENT_SNAPSHOT

#include "eunet/util/error.hpp"
#include "eunet/core/event.hpp"
#include "eunet/core/lifecycle_fsm.hpp"

namespace core
{
    struct EventSnapshot
    {
        Event event;
        int fd;
        LifeState state;
        platform::time::WallPoint ts;
        util::Error error;
    };

}

#endif // INCLUDE_EUNET_CORE_EVENT_SNAPSHOT