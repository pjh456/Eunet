#include "eunet/core/orchestrator.hpp"

#include "eunet/core/event_snapshot.hpp"

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

        auto latest_event_result = timeline.latest_event();
        if (latest_event_result.is_err())
            return EmitResult::Err(latest_event_result.unwrap_err());

        auto latest_event = latest_event_result.unwrap();
        EventSnapshot snap{
            .event = latest_event,
            .fd = e.fd,
            .state = fsm->current_state(),
            .ts = e.ts,
            .has_error = fsm->has_error(),
            .error = fsm->get_last_error()};

        for (auto *sink : sinks)
            sink->on_event(snap);

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