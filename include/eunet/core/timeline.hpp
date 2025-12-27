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
        using EventIdx = std::size_t;
        using IdxList = std::vector<EventIdx>;
        using EvView = const Event *;
        using EvList = std::vector<EvView>;
        template <typename T>
        using QuerySet = std::unordered_map<T, IdxList>;

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
        size_t size() const noexcept;
        size_t count_by_fd(int fd) const;
        size_t count_by_type(EventType type) const;
        size_t count_by_time(
            platform::time::WallPoint start,
            platform::time::WallPoint end) const;

    public:
        bool has_type(EventType type) const;
        util::Result<size_t, Error> sort_by_time();

    public:
        util::Result<EventIdx, Error> push(const Event &e);
        util::Result<size_t, Error> push(const std::vector<Event> &arr);

    public:
        util::Result<size_t, Error> remove_by_fd(int fd);
        util::Result<size_t, Error> remove_by_type(EventType type);
        util::Result<size_t, Error> remove_by_time(
            platform::time::WallPoint start,
            platform::time::WallPoint end);

    public:
        util::Result<EvList, Error> replay_all() const;
        util::Result<EvList, Error> replay_by_fd(int fd) const;
        util::Result<EvList, Error> replay_since(
            platform::time::WallPoint ts) const;

    public:
        const std::vector<Event> &all_events() const;

        util::Result<EvList, Error>
        query_by_fd(int fd) const;
        util::Result<EvList, Error>
        query_by_type(EventType type) const;
        util::Result<EvList, Error>
        query_by_time(
            platform::time::WallPoint start,
            platform::time::WallPoint end) const;
        util::Result<EvList, Error>
        query_errors() const;

    public:
        util::Result<EvView, Error> latest_event() const;
        util::Result<EvView, Error> latest_by_fd(int fd) const;
        util::Result<EvView, Error> latest_by_type(EventType type) const;
    };
}

#endif // INCLUDE_EUNET_CORE_TIMELINE