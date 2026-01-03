#ifndef INCLUDE_EUNET_CORE_EVENT_SNAPSHOT
#define INCLUDE_EUNET_CORE_EVENT_SNAPSHOT

#include <optional>

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
        std::optional<util::Error> error = std::nullopt;
    };

}

#endif // INCLUDE_EUNET_CORE_EVENT_SNAPSHOT