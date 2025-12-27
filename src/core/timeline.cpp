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

    Timeline::EvCnt Timeline::size() const noexcept { return events.size(); }

    Timeline::EvCnt
    Timeline::count_by_fd(int fd) const
    {
        std::lock_guard lock(mtx);
        auto it = fd_index.find(fd);
        return it != fd_index.end()
                   ? it->second.size()
                   : 0UL;
    }

    Timeline::EvCnt
    Timeline::count_by_type(EventType type) const
    {
        std::lock_guard lock(mtx);
        auto it = type_index.find(type);
        return it != type_index.end()
                   ? it->second.size()
                   : 0UL;
    }

    Timeline::EvCnt
    Timeline::count_by_time(
        TimeStamp start,
        TimeStamp end) const
    {
        std::lock_guard lock(mtx);

        if (start > end)
            return 0UL;

        auto comp = [](const Event &e, const TimeStamp &t)
        { return e.ts < t; };

        auto it_start = std::lower_bound(
            events.begin(), events.end(), start,
            [](const Event &e, const TimeStamp &t)
            { return e.ts < t; });
        auto it_end = std::upper_bound(
            events.begin(), events.end(), end,
            [](const TimeStamp &t, const Event &e)
            { return t < e.ts; });
        return it_end - it_start;
    }

    bool Timeline::has_type(EventType type) const
    {
        std::lock_guard lock(mtx);
        return type_index.find(type) != type_index.end();
    }

    Timeline::EvCntResult
    Timeline::sort_by_time()
    {
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

        return EvCntResult::Ok(events.size());
    }

    Timeline::EvIdxResult
    Timeline::push(const Event &e)
    {
        std::lock_guard lock(mtx);

        events.push_back(e);
        size_t idx = events.size() - 1;

        if (e.fd >= 0)
            fd_index[e.fd].push_back(idx);

        type_index[e.type].push_back(idx);

        return EvIdxResult::Ok(idx);
    }

    Timeline::EvCntResult
    Timeline::push(const std::vector<Event> &arr)
    {
        std::lock_guard lock(mtx);

        EvCnt count = 0;
        events.reserve(events.size() + arr.size());
        for (const auto &e : arr)
        {
            events.push_back(e);
            EvIdx idx = events.size() - 1;
            if (e.fd >= 0)
                fd_index[e.fd].push_back(idx);
            type_index[e.type].push_back(idx);
            ++count;
        }

        return EvCntResult::Ok(count);
    }

    Timeline::EvCntResult
    Timeline::remove_by_fd(int fd)
    {
        std::lock_guard lock(mtx);

        auto it = fd_index.find(fd);
        if (it == fd_index.end())
            return EvCntResult::Err(
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

        return EvCntResult::Ok(count);
    }

    Timeline::EvCntResult
    Timeline::remove_by_type(EventType type)
    {
        std::lock_guard lock(mtx);

        auto it = type_index.find(type);
        if (it == type_index.end())
            return EvCntResult::Ok(0);

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

        return EvCntResult::Ok(count);
    }

    Timeline::EvCntResult
    Timeline::remove_by_time(
        TimeStamp start,
        TimeStamp end)
    {
        std::lock_guard lock(mtx);

        if (start > end)
            return EvCntResult::Err(
                Error{
                    .code = 2,
                    .message = "Invalid time range",
                });

        EvCnt count = 0;
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

        return EvCntResult::Ok(count);
    }

    Timeline::EvListResult
    Timeline::replay_all() const
    {
        std::lock_guard lock(mtx);

        EvList result;
        result.reserve(events.size());
        for (const auto &e : events)
            result.push_back(&e);

        return EvListResult::Ok(result);
    }

    Timeline::EvListResult
    Timeline::replay_by_fd(int fd) const
    {
        return query_by_fd(fd);
    }

    Timeline::EvListResult
    Timeline::replay_since(TimeStamp ts) const
    {
        std::lock_guard lock(mtx);

        auto it_start = std::lower_bound(
            events.begin(), events.end(), ts,
            [](const Event &e, TimeStamp t)
            { return e.ts < t; });

        EvList result;
        result.reserve(events.end() - it_start);
        for (auto it = it_start; it != events.end(); ++it)
            result.push_back(&(*it));

        return EvListResult::Ok(result);
    }

    const std::vector<Event> &Timeline::all_events() const
    {
        std::lock_guard lock(mtx);
        return events;
    }

    Timeline::EvListResult
    Timeline::query_by_fd(int fd) const
    {
        std::lock_guard lock(mtx);

        auto it = fd_index.find(fd);
        if (it == fd_index.end())
            return EvListResult::Err(
                Error{
                    .code = 1,
                    .message = "fd not found in timeline",
                });

        EvList result;
        result.reserve(it->second.size());
        for (auto idx : it->second)
            if (idx < events.size())
                result.push_back(&events[idx]);

        return EvListResult::Ok(result);
    }

    Timeline::EvListResult
    Timeline::query_by_type(EventType type) const
    {
        std::lock_guard lock(mtx);

        auto it = type_index.find(type);
        if (it == type_index.end())
            return EvListResult::Err(
                Error{
                    .code = 2,
                    .message = "EventType not found in timeline",
                });

        EvList result;
        result.reserve(it->second.size());
        for (auto idx : (*it).second)
            if (idx < events.size())
                result.push_back(&events[idx]);

        return EvListResult::Ok(result);
    }

    Timeline::EvListResult
    Timeline::query_by_time(
        TimeStamp start,
        TimeStamp end) const
    {
        std::lock_guard lock(mtx);
        if (start > end)
            return EvListResult::Err(
                Error{
                    .code = 3,
                    .message = "Invalid time range: start > end",
                });

        EvList result;

        auto it_start = std::lower_bound(
            events.begin(), events.end(), start,
            [](const Event &e, const TimeStamp &t)
            { return e.ts < t; });
        auto it_end = std::upper_bound(
            events.begin(), events.end(), end,
            [](const TimeStamp &t, const Event &e)
            { return t < e.ts; });

        result.reserve(it_end - it_start);
        for (auto it = it_start; it != it_end; ++it)
            result.push_back(&(*it));

        return EvListResult::Ok(result);
    }

    Timeline::EvListResult
    Timeline::query_errors() const
    {
        return query_by_type(EventType::ERROR);
    }

    Timeline::EvViewResult
    Timeline::latest_event() const
    {
        std::lock_guard lock(mtx);

        if (events.empty())
            return EvViewResult::Err(
                Error{
                    .code = 4,
                    .message = "No events",
                });

        return EvViewResult::Ok(&events.back());
    }

    Timeline::EvViewResult
    Timeline::latest_by_fd(int fd) const
    {
        auto res = query_by_fd(fd);

        std::lock_guard lock(mtx);

        if (res.is_err() || res.unwrap().empty())
            return EvViewResult::Err(
                Error{
                    .code = 5,
                    .message = "No events of fd",
                });

        auto &list = res.unwrap();
        return EvViewResult::Ok(list.back());
    }

    Timeline::EvViewResult
    Timeline::latest_by_type(EventType type) const
    {
        auto res = query_by_type(type);

        std::lock_guard lock(mtx);

        if (res.is_err() || res.unwrap().empty())
            return EvViewResult::Err(
                Error{
                    .code = 6,
                    .message = "No events of type",
                });

        auto &list = res.unwrap();
        return EvViewResult::Ok(list.back());
    }

}