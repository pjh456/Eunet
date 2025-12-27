#include "eunet/core/orchestrator.hpp"

#include <algorithm>

namespace core
{
    const Timeline &
    Orchestrator::get_timeline() const noexcept { return timeline; }
    const LifecycleFSM *
    Orchestrator::get_fsm(int fd) const { return fsm_manager.get(fd); }

    Orchestrator::EmitResult
    Orchestrator::emit(Event e)
    {
        std::lock_guard lock(mtx);

        auto idx_res = timeline.push(e);
        if (!idx_res.is_ok())
            return EmitResult::Err(idx_res.unwrap_err());

        fsm_manager.on_event(e);

        const auto *fsm = fsm_manager.get(e.fd);
        if (!fsm)
            return EmitResult::Err(
                EventError{
                    .domain = "orchestrator",
                    .message = "FSM missing after event commit",
                });

        for (auto *sink : sinks)
            sink->on_event(e, *fsm);

        return EmitResult::Ok();
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