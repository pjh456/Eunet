#ifndef INCLUDE_EUNET_CORE_ORCHESTRATOR
#define INCLUDE_EUNET_CORE_ORCHESTRATOR

#include <vector>
#include <mutex>

#include "eunet/util/result.hpp"
#include "eunet/core/timeline.hpp"
#include "eunet/core/lifecycle_fsm.hpp"
#include "eunet/core/i_event_sink.hpp"

namespace core
{
    class Orchestrator
    {
    public:
        using EmitResult = util::Result<bool, EventError>;

    private:
        Timeline timeline;
        FsmManager fsm_manager;

        std::vector<sink::IEventSink *> sinks;

        mutable std::mutex mtx;

    public:
        Orchestrator() = default;

    public:
        const Timeline &get_timeline() const noexcept;
        const LifecycleFSM *get_fsm(int fd) const;

    public:
        EmitResult emit(Event e);

        void attach(sink::IEventSink *sink);
        void detach(sink::IEventSink *sink);
    };
}

#endif // INCLUDE_EUNET_CORE_ORCHESTRATOR