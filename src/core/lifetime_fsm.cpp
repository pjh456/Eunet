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

    bool LifecycleFSM::has_error() const noexcept { return !last_error.is_ok(); }
    util::Error
    LifecycleFSM::get_last_error() const noexcept { return last_error; }

    void LifecycleFSM::on_event(const Event &e)
    {
        if (fd < 0)
            fd = e.fd.fd;

        last_ts = e.ts;
        if (state == LifeState::Init)
            start_ts = e.ts;

        if (e.is_error())
        {
            last_error = e.error;
            transit(LifeState::Error);
            return;
        }

        switch (state)
        {
        case LifeState::Init:
            if (e.type == EventType::DNS_RESOLVE_START)
                transit(LifeState::Resolving);
            else if (e.type == EventType::TCP_CONNECT_START)
                transit(LifeState::Connecting);
            break;

        case LifeState::Resolving:
            if (e.type == EventType::DNS_RESOLVE_DONE)
                transit(LifeState::Connecting);
            break;

        case LifeState::Connecting:
            if (e.type == EventType::TCP_CONNECT_SUCCESS)
                transit(LifeState::Established);
            break;

        case LifeState::Established:
            if (e.type == EventType::HTTP_SENT)
                transit(LifeState::Sending);
            break;

        case LifeState::Sending:
            if (e.type == EventType::HTTP_RECEIVED)
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
        // if (!e.fd)
        //     return;
        // 允许存在无关联 fd 的事件

        std::lock_guard<std::mutex> lock(mtx);

        auto it = fsms.find(e.fd.fd);
        if (it == fsms.end())
            it = fsms.emplace(e.fd.fd, LifecycleFSM{e.fd.fd}).first;

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