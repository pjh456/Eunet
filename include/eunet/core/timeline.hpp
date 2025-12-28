#ifndef INCLUDE_EUNET_CORE_TIMELINE
#define INCLUDE_EUNET_CORE_TIMELINE

#include <vector>
#include <unordered_map>
#include <mutex>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/core/event.hpp"

namespace core
{
    class Timeline
    {
    public:
        using EvIdx = std::size_t;
        using EvCnt = std::size_t;
        using TimeStamp = platform::time::WallPoint;

        using IdxList = std::vector<EvIdx>;
        using EvList = std::vector<Event>;
        template <typename T>
        using QuerySet = std::unordered_map<T, IdxList>;

        using EvIdxResult = util::ResultV<EvIdx>;
        using EvCntResult = util::ResultV<EvCnt>;
        using EvListResult = util::ResultV<EvList>;
        using EvResult = util::ResultV<Event>;

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

        bool has_type(EventType type) const noexcept;

        EvCntResult sort_by_time();

    public:
        EvIdxResult push(const Event &e);
        EvCntResult push(const std::vector<Event> &arr);

    public:
        EvCnt remove_by_fd(int fd);
        EvCnt remove_by_type(EventType type);
        EvCnt remove_by_time(TimeStamp start, TimeStamp end);

    public:
        EvList replay_all() const;
        EvList replay_by_fd(int fd) const;
        EvList replay_since(TimeStamp ts) const;

    public:
        EvList query_by_fd(int fd) const;
        EvList query_by_type(EventType type) const;
        EvList query_by_time(TimeStamp start, TimeStamp end) const;

        EvList query_errors() const;

    public:
        EvResult latest_event() const;
        EvResult latest_by_fd(int fd) const;
        EvResult latest_by_type(EventType type) const;

    private:
        EvList query_by_fd_locked(int fd) const;
        EvList query_by_type_locked(EventType type) const;
        EvList query_by_time_locked(TimeStamp start, TimeStamp end) const;

    private:
        void rebuild_indexes_locked(const std::vector<Event> &arr);

        template <typename Pred>
        EvCnt remove_if_locked(Pred pred)
        {
            EvCnt removed = 0;

            std::vector<Event> new_events;
            new_events.reserve(events.size());

            for (auto &e : events)
            {
                if (pred(e))
                    ++removed;
                else
                    new_events.push_back(std::move(e));
            }

            rebuild_indexes_locked(new_events);
            events.swap(new_events);
            return removed;
        }

        EvCnt remove_by_fd_locked(platform::fd::FdView fd);
        EvCnt remove_by_type_locked(EventType type);
        EvCnt remove_by_time_locked(TimeStamp start, TimeStamp end);
    };
}

#endif // INCLUDE_EUNET_CORE_TIMELINE