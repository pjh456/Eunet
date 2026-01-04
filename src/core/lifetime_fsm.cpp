/*
 * ============================================================================
 *  File Name   : lifecycle_fsm.cpp
 *  Module      : core
 *
 *  Description :
 *      生命周期状态机实现。核心逻辑是 on_event 方法，根据接收到的事件类型
 *      和当前状态，计算下一个状态并更新时间戳。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#include "eunet/core/lifecycle_fsm.hpp"

#define STATE_CASE(name)        \
    case core::LifeState::name: \
        return #name

namespace core
{
    LifecycleFSM::LifecycleFSM(int fd)
        : fd(fd), last_error(std::nullopt) {}

    int LifecycleFSM::current_fd() const noexcept { return fd; }
    LifeState LifecycleFSM::current_state() const noexcept { return state; }

    LifecycleFSM::TimeStamp
    LifecycleFSM::start_timestamp() const noexcept { return start_ts; }
    LifecycleFSM::TimeStamp
    LifecycleFSM::last_timestamp() const noexcept { return last_ts; }

    bool LifecycleFSM::has_error() const noexcept { return last_error.has_value(); }
    std::optional<util::Error>
    LifecycleFSM::get_last_error() const noexcept { return last_error; }

    void LifecycleFSM::on_event(const Event &e)
    {
        if (fd < 0)
            fd = e.fd.fd;

        last_ts = e.ts;
        if (state == LifeState::Init)
            start_ts = e.ts;

        // -------- 全局错误处理 --------
        if (e.is_error())
        {
            last_error = e.error.value();
            transit(LifeState::Error);
            return;
        }

        switch (state)
        {
        case LifeState::Init:
            switch (e.type)
            {
            case EventType::DNS_RESOLVE_START:
                transit(LifeState::Resolving);
                break;
            case EventType::TCP_CONNECT_START:
                transit(LifeState::Connecting);
                break;
            default:
                break;
            }
            break;

        case LifeState::Resolving:
            if (e.type == EventType::DNS_RESOLVE_DONE)
                transit(LifeState::Connecting);
            break;

        case LifeState::Connecting:
            switch (e.type)
            {
            case EventType::TCP_CONNECT_SUCCESS:
                // 是否启用 TLS，通常由 config 决定
                // if (use_tls)
                //     transit(LifeState::Handshaking);
                // else
                transit(LifeState::Established);
                break;

            case EventType::TCP_CONNECT_TIMEOUT:
                transit(LifeState::Error);
                break;

            default:
                break;
            }
            break;

        case LifeState::Handshaking:
            switch (e.type)
            {
            case EventType::TLS_HANDSHAKE_DONE:
                transit(LifeState::Established);
                break;
            default:
                break;
            }
            break;

        case LifeState::Established:
            if (e.type == EventType::HTTP_REQUEST_BUILD ||
                e.type == EventType::HTTP_SENT)
            {
                transit(LifeState::Sending);
            }
            break;

        case LifeState::Sending:
            if (e.type == EventType::HTTP_SENT)
                transit(LifeState::Receiving);
            break;

        case LifeState::Receiving:
            switch (e.type)
            {
            case EventType::HTTP_HEADERS_RECEIVED:
                // still Receiving
                break;
            case EventType::HTTP_BODY_DONE:
            case EventType::CONNECTION_CLOSED:
                transit(LifeState::Finished);
                break;
            default:
                break;
            }
            break;

        case LifeState::Finished:
        case LifeState::Error:
            // 终态，忽略后续事件
            break;
        }
    }

    void LifecycleFSM::transit(LifeState next) noexcept { state = next; }

    const LifecycleFSM *FsmManager::get(SessionId sid) const
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = fsms.find(sid);
        return it == fsms.end() ? nullptr : &it->second;
    }

    bool FsmManager::has(int fd) const noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        return fsms.find(fd) != fsms.end();
    }

    std::size_t FsmManager::size() const noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        return fsms.size();
    }

    void FsmManager::on_event(const Event &e)
    {
        // if (!e.fd)
        //     return;
        // 允许存在无关联 fd 的事件

        std::lock_guard lock(mtx);

        SessionId key = e.session_id;
        auto it = fsms.find(e.fd.fd);
        if (it == fsms.end())
            it = fsms.emplace(key, LifecycleFSM{e.fd.fd}).first;

        fsms[key].on_event(e);
    }

    void FsmManager::clear()
    {
        fsms.clear();
    }
}

std::string to_string(core::LifeState s) noexcept
{
    using namespace core;
    switch (s)
    {
        STATE_CASE(Init);
        STATE_CASE(Resolving);
        STATE_CASE(Connecting);
        STATE_CASE(Handshaking);
        STATE_CASE(Established);
        STATE_CASE(Sending);
        STATE_CASE(Receiving);
        STATE_CASE(Finished);
        STATE_CASE(Error);
    default:
        return "Unknown";
    }
}