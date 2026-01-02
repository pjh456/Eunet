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
        using Ret = EmitResult;
        using util::Error;

        std::lock_guard lock(mtx);

        // 1. 如果事件没有 SessionId 且关联了 FD，尝试从 FSM 恢复或分配
        // （此处简化处理，实际建议在 Client 创建时就分配好 ID）

        // 2. 更新 Timeline
        auto idx_res = timeline.push(e);
        if (!idx_res.is_ok())
        {
            return Ret::Err(
                Error::internal()
                    .resource_exhausted() // 假设是内存分配失败导致 push 失败
                    .message("Failed to append event to timeline")
                    .wrap(idx_res.unwrap_err())
                    .build());
        }

        // 3. 更新 FSM（注意：FsmManager 内部也需要同步）
        fsm_manager.on_event(e);

        // 4. 快照并分发
        const auto *fsm = fsm_manager.get(e.session_id); // 改用 session_id 索引
        if (!fsm)
        {
            return Ret::Err(
                Error::internal()
                    .invalid_state()
                    .fatal()
                    .message("FSM instance missing for active session")
                    .context(std::to_string(e.session_id))
                    .build());
        }

        auto latest_event_result = timeline.latest_event();
        if (latest_event_result.is_err())
        {
            return Ret::Err(
                Error::internal()
                    .message("Failed to fetch latest event after commit")
                    .context("orchestrator::emit")
                    .wrap(latest_event_result.unwrap_err())
                    .build());
        }

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

        return Ret::Ok();
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