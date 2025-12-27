#ifndef INCLUDE_EUNET_CORE_LIFECYCLE_FSM
#define INCLUDE_EUNET_CORE_LIFECYCLE_FSM

#include <vector>
#include <mutex>
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
        Connecting,  // TCP_CONNECT 已发生
        Established, // TCP_ESTABLISHED
        Sending,     // REQUEST_SENT
        Receiving,   // REQUEST_RECEIVED
        Finished,    // 正常完成
        Error        // 任意错误
    };

    class LifecycleFSM
    {
    public:
        using TimeStamp = platform::time::WallPoint;

    private:
        int fd{-1};

        LifeState state{LifeState::Init};
        TimeStamp start_ts{};
        TimeStamp last_ts{};

        std::optional<EventError> last_error;

    public:
        explicit LifecycleFSM(int fd = -1);

    public:
        int current_fd() const noexcept;
        LifeState current_state() const noexcept;

        TimeStamp start_timestamp() const noexcept;
        TimeStamp last_timestamp() const noexcept;

        bool has_error() const noexcept;
        const EventError *get_last_error() const noexcept;

    public:
        void on_event(const Event &e);

    private:
        void transit(LifeState next) noexcept;
    };

    class FsmManager
    {
    private:
        mutable std::mutex mtx;
        std::unordered_map<int, LifecycleFSM> fsms;

    public:
        FsmManager() = default;

    public:
        const LifecycleFSM *get(int fd) const;
        bool has(int fd) const noexcept;

        std::size_t size() const noexcept;

    public:
        void on_event(const Event &e);
    };
}

const char *to_string(core::LifeState s) noexcept;

#endif // INCLUDE_EUNET_CORE_LIFECYCLE_FSM