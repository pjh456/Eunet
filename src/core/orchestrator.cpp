/*
 * ============================================================================
 *  File Name   : orchestrator.cpp
 *  Module      : core
 *
 *  Description :
 *      Orchestrator 实现。线程安全地接收 emit 请求，顺序更新 Timeline 和
 *      FSM，构建 Snapshot 并分发给所有注册的 Sink。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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

        // 加锁 保护 Timeline 和 FSM 以及 Sink 列表
        std::lock_guard lock(mtx);

        // 将事件追加到 Timeline 数据库中
        auto idx_res = timeline.push(e);
        if (!idx_res.is_ok())
        {
            // 记录 Timeline 写入失败通常意味着内存不足
            return Ret::Err(
                Error::internal()
                    .resource_exhausted()
                    .message("Failed to append event to timeline")
                    .context("Orchestrator::emit")
                    .wrap(idx_res.unwrap_err())
                    .build());
        }

        // 将事件输入状态机管理器 更新对应 Session 的生命周期状态
        fsm_manager.on_event(e);

        // 获取当前 Session 的状态机实例 如果是新建的则可能需要查找
        const auto *fsm = fsm_manager.get(e.session_id);

        // 从 Timeline 取回刚刚存入的事件以确保一致性
        auto latest_event_result = timeline.latest_event();
        if (latest_event_result.is_err())
        {
            return Ret::Err(
                Error::internal()
                    .message("Failed to fetch latest event after commit")
                    .context("Orchestrator::emit")
                    .wrap(latest_event_result.unwrap_err())
                    .build());
        }

        // 构建事件快照 包含原始事件、当前状态、累积错误等
        // 快照是不可变的数据结构 适合跨线程传递给 UI
        EventSnapshot snap{
            .event = e,
            .fd = e.fd.fd,
            .state = fsm ? fsm->current_state() : LifeState::Finished,
            .ts = e.ts,
            .error = fsm ? fsm->get_last_error() : std::nullopt,
            .payload = e.payload,
        };

        // 遍历所有注册的 Sink 分发快照
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

    void Orchestrator::reset()
    {
        std::lock_guard lock(mtx);
        timeline.clear();
        fsm_manager.clear();
    }
}