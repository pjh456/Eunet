/*
 * ============================================================================
 *  File Name   : orchestrator.hpp
 *  Module      : core
 *
 *  Description :
 *      系统编排器（中枢）。集成 Timeline、FsmManager 和 Sinks。
 *      提供统一的 `emit` 接口供网络模块上报事件，处理后分发给 UI 等观察者。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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
    /**
     * @brief 核心编排器
     *
     * 系统的中枢神经。负责接收来自 Network 层的事件，
     * 将其写入 Timeline，更新 FSM 状态，并将快照分发给所有注册的 Sink（如 UI）。
     * 保证了事件处理的线性化和线程安全。
     */
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
        /**
         * @brief 提交一个新事件
         *
         * 此操作是线程安全的。它会触发从存储到通知的一系列流程。
         *
         * @param e 待提交的事件
         * @return EmitResult 提交结果
         */
        EmitResult emit(Event e);

        SessionId new_session() { return next_session_id_.fetch_add(1); }

        void attach(SinkPtr sink);
        void detach(SinkPtr sink);
        void reset();
    };
}

#endif // INCLUDE_EUNET_CORE_ORCHESTRATOR