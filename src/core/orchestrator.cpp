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

        // 1. 如果事件没有 SessionId 且关联了 FD，尝试从 FSM 恢复或分配
        // （此处简化处理，实际建议在 Client 创建时就分配好 ID）

        // 2. 更新 Timeline
        auto idx_res = timeline.push(e);
        if (!idx_res.is_ok())
            return EmitResult::Err(idx_res.unwrap_err());

        // 3. 更新 FSM（注意：FsmManager 内部也需要同步）
        fsm_manager.on_event(e);

        // 4. 快照并分发
        const auto *fsm = fsm_manager.get(e.session_id); // 改用 session_id 索引
        if (!fsm)
            return EmitResult::Err(
                util::Error::internal(
                    "FSM missing after event commit"));

        auto latest_event_result = timeline.latest_event();
        if (latest_event_result.is_err())
            return EmitResult::Err(latest_event_result.unwrap_err());

        EventSnapshot snap{
            .event = e,
            .fd = e.fd.fd,
            .state = fsm->current_state(),
            .ts = e.ts,
            .error = fsm->get_last_error()};

        for (auto &sink : sinks)
        {
            if (sink)
                sink->on_event(snap);
        }

        return EmitResult::Ok();
    }

    void Orchestrator::attach(SinkPtr sink)
    {
        std::lock_guard lock(mtx);
        if (sink)
            sinks.push_back(sink);
    }

    void Orchestrator::detach(SinkPtr sink)
    {
        std::lock_guard lock(mtx);
        if (sink)
            sinks.erase(
                std::remove(sinks.begin(), sinks.end(), sink),
                sinks.end());
    }
}