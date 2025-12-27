#include "eunet/core/timeline.hpp"

#include <algorithm>

namespace core
{
    void Timeline::clear()
    {
        std::lock_guard lock(mtx);

        events.clear();
        fd_index.clear();
        type_index.clear();
    }

    size_t Timeline::size() const noexcept { return events.size(); }

    size_t Timeline::count_by_fd(int fd) const
    {
        std::lock_guard lock(mtx);
        auto it = fd_index.find(fd);
        return it != fd_index.end() ? it->second.size() : 0UL;
    }

    size_t Timeline::count_by_type(EventType type) const
    {
        std::lock_guard lock(mtx);
        auto it = type_index.find(type);
        return it != type_index.end() ? it->second.size() : 0UL;
    }

    size_t Timeline::count_by_time(
        platform::time::WallPoint start,
        platform::time::WallPoint end) const
    {
        std::lock_guard lock(mtx);

        if (start > end)
            return 0UL;

        auto comp = [](const Event &e, const platform::time::WallPoint &t)
        { return e.ts < t; };

        auto it_start = std::lower_bound(
            events.begin(), events.end(), start,
            [](const Event &e, const platform::time::WallPoint &t)
            { return e.ts < t; });
        auto it_end = std::upper_bound(
            events.begin(), events.end(), end,
            [](const platform::time::WallPoint &t, const Event &e)
            { return t < e.ts; });
        return it_end - it_start;
    }

    bool Timeline::has_type(EventType type) const
    {
        std::lock_guard lock(mtx);
        return type_index.find(type) != type_index.end();
    }

    util::Result<size_t, Error> Timeline::sort_by_time()
    {
        using Result = util::Result<size_t, Error>;

        std::lock_guard lock(mtx);

        std::sort(
            events.begin(), events.end(),
            [](const Event &a, const Event &b)
            { return a.ts < b.ts; });

        fd_index.clear();
        type_index.clear();
        for (size_t i = 0; i < events.size(); ++i)
        {
            if (events[i].fd >= 0)
                fd_index[events[i].fd].push_back(i);
            type_index[events[i].type].push_back(i);
        }

        return Result::Ok(events.size());
    }

    util::Result<Timeline::EventIdx, Error>
    Timeline::push(const Event &e)
    {
        using Result = util::Result<Timeline::EventIdx, Error>;

        std::lock_guard lock(mtx);

        events.push_back(e);
        size_t idx = events.size() - 1;

        if (e.fd >= 0)
            fd_index[e.fd].push_back(idx);

        type_index[e.type].push_back(idx);

        return Result::Ok(idx);
    }

    util::Result<size_t, Error>
    Timeline::push(const std::vector<Event> &arr)
    {
        using Result = util::Result<size_t, Error>;

        std::lock_guard lock(mtx);

        size_t count = 0;
        events.reserve(events.size() + arr.size());
        for (const auto &e : arr)
        {
            events.push_back(e);
            EventIdx idx = events.size() - 1;
            if (e.fd >= 0)
                fd_index[e.fd].push_back(idx);
            type_index[e.type].push_back(idx);
            ++count;
        }

        return Result::Ok(count);
    }

    util::Result<size_t, Error>
    Timeline::remove_by_fd(int fd)
    {
        using Result = util::Result<size_t, Error>;

        std::lock_guard lock(mtx);

        auto it = fd_index.find(fd);
        if (it == fd_index.end())
            return Result::Err(
                Error{
                    .code = 1,
                    .message = "fd not found",
                });

        IdxList &idxs = it->second;
        int count = 0;

        std::vector<Event> new_events;
        QuerySet<int> new_fd_index;
        QuerySet<EventType> new_type_index;

        for (size_t i = 0; i < events.size(); ++i)
        {
            if (std::find(idxs.begin(), idxs.end(), i) != idxs.end())
                ++count;
            else
            {
                Event &e = events[i];
                size_t new_idx = new_events.size();
                new_events.push_back(e);

                if (e.fd >= 0)
                    new_fd_index[e.fd].push_back(new_idx);
                new_type_index[e.type].push_back(new_idx);
            }
        }

        events = std::move(new_events);
        fd_index = std::move(new_fd_index);
        type_index = std::move(new_type_index);

        return Result::Ok(count);
    }

    util::Result<size_t, Error>
    Timeline::remove_by_type(EventType type)
    {
        using Result = util::Result<size_t, Error>;

        std::lock_guard lock(mtx);

        auto it = type_index.find(type);
        if (it == type_index.end())
            return Result::Ok(0);

        IdxList &idxs = it->second;
        int count = 0;

        std::vector<Event> new_events;
        QuerySet<int> new_fd_index;
        QuerySet<EventType> new_type_index;

        for (size_t i = 0; i < events.size(); ++i)
        {
            if (events[i].type == type)
                ++count;
            else
            {
                Event &e = events[i];
                size_t new_idx = new_events.size();
                new_events.push_back(e);

                if (e.fd >= 0)
                    new_fd_index[e.fd].push_back(new_idx);
                new_type_index[e.type].push_back(new_idx);
            }
        }

        events = std::move(new_events);
        fd_index = std::move(new_fd_index);
        type_index = std::move(new_type_index);

        return Result::Ok(count);
    }

    util::Result<size_t, Error>
    Timeline::remove_by_time(
        platform::time::WallPoint start,
        platform::time::WallPoint end)
    {
        using Result = util::Result<size_t, Error>;

        std::lock_guard lock(mtx);

        if (start > end)
            return Result::Err(
                Error{
                    .code = 2,
                    .message = "Invalid time range",
                });

        size_t count = 0;
        std::vector<Event> new_events;
        QuerySet<int> new_fd_index;
        QuerySet<EventType> new_type_index;

        for (size_t i = 0; i < events.size(); ++i)
        {
            const Event &e = events[i];
            if (e.ts >= start && e.ts <= end)
                ++count;
            else
            {
                size_t new_idx = new_events.size();
                new_events.push_back(e);
                if (e.fd >= 0)
                    new_fd_index[e.fd].push_back(new_idx);
                new_type_index[e.type].push_back(new_idx);
            }
        }

        events = std::move(new_events);
        fd_index = std::move(new_fd_index);
        type_index = std::move(new_type_index);

        return Result::Ok(count);
    }

    util::Result<Timeline::EvList, Error>
    Timeline::replay_all() const
    {
        using Result = util::Result<Timeline::EvList, Error>;

        std::lock_guard lock(mtx);

        EvList result;
        result.reserve(events.size());
        for (const auto &e : events)
            result.push_back(&e);

        return Result::Ok(result);
    }

    util::Result<Timeline::EvList, Error>
    Timeline::replay_by_fd(int fd) const
    {
        return query_by_fd(fd);
    }

    util::Result<Timeline::EvList, Error>
    Timeline::replay_since(platform::time::WallPoint ts) const
    {
        using Result = util::Result<Timeline::EvList, Error>;

        std::lock_guard lock(mtx);

        auto it_start = std::lower_bound(
            events.begin(), events.end(), ts,
            [](const Event &e, platform::time::WallPoint t)
            { return e.ts < t; });

        EvList result;
        result.reserve(events.end() - it_start);
        for (auto it = it_start; it != events.end(); ++it)
            result.push_back(&(*it));

        return Result::Ok(result);
    }

    const std::vector<Event> &Timeline::all_events() const
    {
        std::lock_guard lock(mtx);
        return events;
    }

    util::Result<Timeline::EvList, Error>
    Timeline::query_by_fd(int fd) const
    {
        using Result = util::Result<EvList, Error>;

        std::lock_guard lock(mtx);

        auto it = fd_index.find(fd);
        if (it == fd_index.end())
            return Result::Err(
                Error{
                    .code = 1,
                    .message = "fd not found in timeline",
                });

        EvList result;
        result.reserve(it->second.size());
        for (auto idx : it->second)
            if (idx < events.size())
                result.push_back(&events[idx]);

        return Result::Ok(result);
    }

    util::Result<Timeline::EvList, Error>
    Timeline::query_by_type(EventType type) const
    {
        using Result = util::Result<EvList, Error>;

        std::lock_guard lock(mtx);

        auto it = type_index.find(type);
        if (it == type_index.end())
            return util::Result<EvList, Error>::Err(
                Error{
                    .code = 2,
                    .message = "EventType not found in timeline",
                });

        EvList result;
        result.reserve(it->second.size());
        for (auto idx : (*it).second)
            if (idx < events.size())
                result.push_back(&events[idx]);

        return Result::Ok(result);
    }

    util::Result<Timeline::EvList, Error>
    Timeline::query_by_time(
        platform::time::WallPoint start,
        platform::time::WallPoint end) const
    {
        using Result = util::Result<EvList, Error>;

        std::lock_guard lock(mtx);
        if (start > end)
            return Result::Err(
                Error{
                    .code = 3,
                    .message = "Invalid time range: start > end",
                });

        EvList result;

        auto it_start = std::lower_bound(
            events.begin(), events.end(), start,
            [](const Event &e, const platform::time::WallPoint &t)
            { return e.ts < t; });
        auto it_end = std::upper_bound(
            events.begin(), events.end(), end,
            [](const platform::time::WallPoint &t, const Event &e)
            { return t < e.ts; });

        result.reserve(it_end - it_start);
        for (auto it = it_start; it != it_end; ++it)
            result.push_back(&(*it));

        return Result::Ok(result);
    }

    util::Result<Timeline::EvList, Error> Timeline::query_errors() const
    {
        return query_by_type(EventType::ERROR);
    }

    util::Result<Timeline::EvView, Error> Timeline::latest_event() const
    {
        using Result = util::Result<EvView, Error>;

        std::lock_guard lock(mtx);

        if (events.empty())
            return Result::Err(
                Error{
                    .code = 4,
                    .message = "No events",
                });

        return Result::Ok(&events.back());
    }

    util::Result<Timeline::EvView, Error>
    Timeline::latest_by_fd(int fd) const
    {
        using Result = util::Result<EvView, Error>;

        auto res = query_by_fd(fd);

        std::lock_guard lock(mtx);

        if (res.is_err() || res.unwrap().empty())
            return Result::Err(
                Error{
                    .code = 5,
                    .message = "No events of fd",
                });

        auto &list = res.unwrap();
        return Result::Ok(list.back());
    }

    util::Result<Timeline::EvView, Error>
    Timeline::latest_by_type(EventType type) const
    {
        using Result = util::Result<EvView, Error>;

        auto res = query_by_type(type);

        std::lock_guard lock(mtx);

        if (res.is_err() || res.unwrap().empty())
            return Result::Err(
                Error{
                    .code = 6,
                    .message = "No events of type",
                });

        auto &list = res.unwrap();
        return Result::Ok(list.back());
    }

}