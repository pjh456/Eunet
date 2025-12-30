#ifndef INCLUDE_EUNET_CORE_ORCHESTRATOR
#define INCLUDE_EUNET_CORE_ORCHESTRATOR

#include <vector>
#include <mutex>
#include <memory>
#include <atomic>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/core/timeline.hpp"
#include "eunet/core/lifecycle_fsm.hpp"
#include "eunet/core/sink.hpp"

namespace core
{

    class Orchestrator
    {
    public:
        using EmitResult = util::ResultV<void>;
        using SinkPtr = std::shared_ptr<sink::IEventSink>;

    private:
        Timeline timeline;
        FsmManager fsm_manager;

        std::vector<SinkPtr> sinks;
        std::atomic<SessionId> next_session_id_{1};
        mutable std::mutex mtx;

    public:
        Orchestrator() = default;

    public:
        const Timeline &get_timeline() const noexcept;
        const LifecycleFSM *get_fsm(int fd) const;

    public:
        EmitResult emit(Event e);

        SessionId new_session() { return next_session_id_.fetch_add(1); }

        void attach(SinkPtr sink);
        void detach(SinkPtr sink);
    };
}

#endif // INCLUDE_EUNET_CORE_ORCHESTRATOR