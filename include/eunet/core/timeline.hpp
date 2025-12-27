#ifndef INCLUDE_EUNET_CORE_TIMELINE
#define INCLUDE_EUNET_CORE_TIMELINE

#include <vector>
#include <unordered_map>
#include <mutex>

#include "eunet/util/result.hpp"
#include "eunet/core/event.hpp"

namespace core
{
    class Timeline
    {
    public:
        using EvIdx = std::size_t;
        using EvCnt = std::size_t;
        using EvView = const Event *;
        using TimeStamp = platform::time::WallPoint;

        using IdxList = std::vector<EvIdx>;
        using EvList = std::vector<EvView>;
        template <typename T>
        using QuerySet = std::unordered_map<T, IdxList>;

        using EvIdxResult = util::Result<EvIdx, Error>;
        using EvCntResult = util::Result<EvCnt, Error>;
        using EvListResult = util::Result<EvList, Error>;
        using EvViewResult = util::Result<EvView, Error>;

    private:
        std::vector<Event> events;
        QuerySet<int> fd_index;
        QuerySet<EventType> type_index;
        mutable std::mutex mtx;

    public:
        Timeline() = default;

        Timeline(const Timeline &) = default;
        Timeline &operator=(const Timeline &) = default;

        Timeline(Timeline &&) noexcept = default;
        Timeline &operator=(Timeline &&) noexcept = default;

        ~Timeline() = default;

    public:
        void clear();
        EvCnt size() const noexcept;
        EvCnt count_by_fd(int fd) const;
        EvCnt count_by_type(EventType type) const;
        EvCnt count_by_time(TimeStamp start, TimeStamp end) const;

    public:
        bool has_type(EventType type) const;
        EvCntResult sort_by_time();

    public:
        EvIdxResult push(const Event &e);
        EvCntResult push(const std::vector<Event> &arr);

    public:
        EvCntResult remove_by_fd(int fd);
        EvCntResult remove_by_type(EventType type);
        EvCntResult remove_by_time(TimeStamp start, TimeStamp end);

    public:
        EvListResult replay_all() const;
        EvListResult replay_by_fd(int fd) const;
        EvListResult replay_since(TimeStamp ts) const;

    public:
        const std::vector<Event> &all_events() const;

        EvListResult query_by_fd(int fd) const;
        EvListResult query_by_type(EventType type) const;
        EvListResult query_by_time(TimeStamp start, TimeStamp end) const;
        EvListResult query_errors() const;

    public:
        EvViewResult latest_event() const;
        EvViewResult latest_by_fd(int fd) const;
        EvViewResult latest_by_type(EventType type) const;
    };
}

#endif // INCLUDE_EUNET_CORE_TIMELINE