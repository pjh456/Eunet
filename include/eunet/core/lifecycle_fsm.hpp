/*
 * ============================================================================
 *  File Name   : lifecycle_fsm.hpp
 *  Module      : core
 *
 *  Description :
 *      网络请求生命周期状态机。定义了从 Init -> Resolving -> Connecting
 *      -> Established -> Sending -> Receiving -> Finished 的状态流转逻辑。
 *      FsmManager 负责管理多路并发会话的状态机实例。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_CORE_LIFECYCLE_FSM
#define INCLUDE_EUNET_CORE_LIFECYCLE_FSM

#include <vector>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "eunet/util/result.hpp"
#include "eunet/platform/time.hpp"
#include "eunet/core/event.hpp"

namespace core
{
    enum class LifeState
    {
        Init,        // 尚未开始
        Resolving,   // DNS 中
        Connecting,  // TCP 连接中
        Handshaking, // TLS 握手中（新增）
        Established, // 连接就绪（可收发）
        Sending,     // 请求发送中
        Receiving,   // 响应接收中
        Finished,    // 正常完成（HTTP body done / graceful close）
        Error        // 任意错误
    };

    /**
     * @brief 生命周期状态机
     *
     * 维护单个网络连接的生命周期状态。
     * 根据输入的 Event 流，推导当前连接处于 Init, Connecting, Established 等哪个阶段。
     */
    class LifecycleFSM
    {
    public:
        using TimeStamp = platform::time::WallPoint;

    private:
        int fd{-1};

        LifeState state{LifeState::Init};
        TimeStamp start_ts{};
        TimeStamp last_ts{};

        std::optional<util::Error> last_error;

    public:
        explicit LifecycleFSM(int fd = -1);

    public:
        int current_fd() const noexcept;
        LifeState current_state() const noexcept;

        TimeStamp start_timestamp() const noexcept;
        TimeStamp last_timestamp() const noexcept;

        bool has_error() const noexcept;
        std::optional<util::Error> get_last_error() const noexcept;

    public:
        /**
         * @brief 响应新事件并更新状态
         *
         * 核心状态流转逻辑。
         * 例如：收到 TCP_CONNECT_SUCCESS 事件时，状态从 Connecting 变为 Established。
         *
         * @param e 发生的事件
         */
        void on_event(const Event &e);

    private:
        void transit(LifeState next) noexcept;
    };

    class FsmManager
    {
    private:
        mutable std::mutex mtx;
        std::unordered_map<SessionId, LifecycleFSM> fsms;

    public:
        FsmManager() = default;

    public:
        const LifecycleFSM *get(SessionId sid) const;
        bool has(int fd) const noexcept;

        std::size_t size() const noexcept;

    public:
        void on_event(const Event &e);
        void clear();
    };
}

std::string to_string(core::LifeState s) noexcept;

#endif // INCLUDE_EUNET_CORE_LIFECYCLE_FSM