#include "eunet/core/orchestrator.hpp"

#include <algorithm>

namespace core
{
    const Timeline &
    Orchestrator::get_timeline() const noexcept { return timeline; }
    const LifecycleFSM *
    Orchestrator::get_fsm(int fd) const { return fsm_manager.get(fd); }

    void Orchestrator::emit(Event e)
    {
        std::lock_guard lock(mtx);

        timeline.push(e);
        fsm_manager.on_event(e);

        const auto *fsm = fsm_manager.get(e.fd);

        for (auto *sink : sinks)
            sink->on_event(e, *fsm);
    }

    void Orchestrator::attach(sink::IEventSink *sink)
    {
        if (!sink)
            return;

        sinks.push_back(sink);
    }

    void Orchestrator::detach(sink::IEventSink *sink)
    {
        if (!sink)
            return;

        std::remove(sinks.begin(), sinks.end(), sink);
    }
}