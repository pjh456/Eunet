#include "eunet/core/lifecycle_fsm.hpp"

namespace core
{
    LifecycleFSM::LifecycleFSM(int fd) : fd(fd) {}

    int LifecycleFSM::current_fd() const noexcept { return fd; }
    LifeState LifecycleFSM::current_state() const noexcept { return state; }

    LifecycleFSM::TimeStamp
    LifecycleFSM::start_timestamp() const noexcept { return start_ts; }
    LifecycleFSM::TimeStamp
    LifecycleFSM::last_timestamp() const noexcept { return last_ts; }

    bool LifecycleFSM::has_error() const noexcept { return last_error.has_value(); }
    const EventError *LifecycleFSM::get_last_error() const noexcept { return last_error ? &(*last_error) : nullptr; }

    void LifecycleFSM::on_event(const Event &e)
    {
        if (fd < 0)
            fd = e.fd;

        last_ts = e.ts;
        if (state == LifeState::Init)
            start_ts = e.ts;

        if (e.is_error())
        {
            last_error =
                e.error
                    ? e.error
                    : EventError{"unknown", "unknown error"};
            transit(LifeState::Error);
            return;
        }

        switch (state)
        {
        case LifeState::Init:
            if (e.type == EventType::DNS_START)
                transit(LifeState::Resolving);
            else if (e.type == EventType::TCP_CONNECT)
                transit(LifeState::Connecting);
            break;

        case LifeState::Resolving:
            if (e.type == EventType::DNS_DONE)
                transit(LifeState::Connecting);
            break;

        case LifeState::Connecting:
            if (e.type == EventType::TCP_ESTABLISHED)
                transit(LifeState::Established);
            break;

        case LifeState::Established:
            if (e.type == EventType::REQUEST_SENT)
                transit(LifeState::Sending);
            break;

        case LifeState::Sending:
            if (e.type == EventType::REQUEST_RECEIVED)
                // transit(LifeState::Receiving);
                transit(LifeState::Finished);
            break;

        case LifeState::Receiving:
            transit(LifeState::Finished);
            break;

        case LifeState::Finished:
        case LifeState::Error:
            // 终态，忽略后续事件
            break;
        }
    }

    void LifecycleFSM::transit(LifeState next) noexcept { state = next; }

    const LifecycleFSM *FsmManager::get(int fd) const
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = fsms.find(fd);
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
        if (e.fd < 0)
            return;

        std::lock_guard<std::mutex> lock(mtx);

        auto it = fsms.find(e.fd);
        if (it == fsms.end())
            it = fsms.emplace(e.fd, LifecycleFSM{e.fd}).first;

        it->second.on_event(e);
    }
}

const char *to_string(core::LifeState s) noexcept
{
    using namespace core;
    switch (s)
    {
    case LifeState::Init:
        return "Init";
    case LifeState::Resolving:
        return "Resolving";
    case LifeState::Connecting:
        return "Connecting";
    case LifeState::Established:
        return "Established";
    case LifeState::Sending:
        return "Sending";
    case LifeState::Receiving:
        return "Receiving";
    case LifeState::Finished:
        return "Finished";
    case LifeState::Error:
        return "Error";
    default:
        return "Unknown";
    }
}