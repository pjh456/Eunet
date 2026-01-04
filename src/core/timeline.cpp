/*
 * ============================================================================
 *  File Name   : timeline.cpp
 *  Module      : core
 *
 *  Description :
 *      Timeline 实现。维护事件的主存储 vector 以及基于 FD 和 Type 的
 *      辅助索引 (Hash Map)，提供线程安全的查询、排序和回放功能。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#include "eunet/core/timeline.hpp"

#include <algorithm>
#include <unordered_set>

namespace core
{
    void Timeline::clear()
    {
        std::lock_guard lock(mtx);

        events.clear();
        fd_index.clear();
        type_index.clear();
    }

    Timeline::EvCnt
    Timeline::size() const noexcept { return events.size(); }

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

    bool Timeline::has_type(EventType type) const noexcept
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
            if (events[i].fd)
                fd_index[events[i].fd.fd].push_back(i);
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

        if (e.fd)
            fd_index[e.fd.fd].push_back(idx);

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
            if (e.fd)
                fd_index[e.fd.fd].push_back(idx);
            type_index[e.type].push_back(idx);
            ++count;
        }

        return EvCntResult::Ok(count);
    }

    Timeline::EvCnt
    Timeline::remove_by_fd(int fd)
    {
        std::lock_guard lock(mtx);

        auto it = fd_index.find(fd);
        if (it == fd_index.end())
            return 0UL;

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

                if (e.fd)
                    new_fd_index[e.fd.fd].push_back(new_idx);
                new_type_index[e.type].push_back(new_idx);
            }
        }

        events = std::move(new_events);
        fd_index = std::move(new_fd_index);
        type_index = std::move(new_type_index);

        return count;
    }

    Timeline::EvCnt
    Timeline::remove_by_type(EventType type)
    {
        std::lock_guard lock(mtx);

        auto it = type_index.find(type);
        if (it == type_index.end())
            return 0UL;

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

                if (e.fd)
                    new_fd_index[e.fd.fd].push_back(new_idx);
                new_type_index[e.type].push_back(new_idx);
            }
        }

        events = std::move(new_events);
        fd_index = std::move(new_fd_index);
        type_index = std::move(new_type_index);

        return count;
    }

    Timeline::EvCnt
    Timeline::remove_by_time(
        TimeStamp start,
        TimeStamp end)
    {
        std::lock_guard lock(mtx);

        if (start > end)
            return 0UL;

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
                if (e.fd)
                    new_fd_index[e.fd.fd].push_back(new_idx);
                new_type_index[e.type].push_back(new_idx);
            }
        }

        events = std::move(new_events);
        fd_index = std::move(new_fd_index);
        type_index = std::move(new_type_index);

        return count;
    }

    Timeline::EvList
    Timeline::replay_all() const
    {
        std::lock_guard lock(mtx);

        EvList result;
        result.reserve(events.size());
        for (const auto &e : events)
            result.push_back(e);

        return result;
    }

    Timeline::EvList
    Timeline::replay_by_fd(int fd) const
    {
        std::lock_guard lock(mtx);
        return query_by_fd_locked(fd);
    }

    Timeline::EvList
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
            result.push_back(*it);

        return result;
    }

    Timeline::EvList
    Timeline::query_by_fd(int fd) const
    {
        std::lock_guard lock(mtx);
        return query_by_fd_locked(fd);
    }

    Timeline::EvList
    Timeline::query_by_type(EventType type) const
    {
        std::lock_guard lock(mtx);
        return query_by_type_locked(type);
    }

    Timeline::EvList
    Timeline::query_by_time(
        TimeStamp start,
        TimeStamp end) const
    {
        std::lock_guard lock(mtx);
        return query_by_time_locked(start, end);
    }

    Timeline::EvList
    Timeline::query_errors() const
    {
        std::lock_guard lock(mtx);

        EvList result;
        for (const auto &e : events)
            if (e.error)
                result.push_back(e);

        return result;
    }

    Timeline::EvResult
    Timeline::latest_event() const
    {
        using Ret = EvResult;
        using util::Error;

        std::lock_guard lock(mtx);

        if (events.empty())
            return Ret::Err(
                Error::state()
                    .invalid_state()
                    .message("Cannot fetch latest event: Timeline is empty")
                    .build());

        return Ret::Ok(events.back());
    }

    Timeline::EvResult
    Timeline::latest_by_fd(int fd) const
    {
        using Ret = EvResult;
        using util::Error;

        std::lock_guard lock(mtx);

        auto res = query_by_fd_locked(fd);

        if (res.empty())
        {
            return Ret::Err(
                Error::state()
                    .target_not_found() // 没有找到指定 FD 的事件
                    .message("No events found for specified FD")
                    .context(std::to_string(fd))
                    .build());
        }

        return Ret::Ok(res.back());
    }

    Timeline::EvResult
    Timeline::latest_by_type(EventType type) const
    {
        using Ret = EvResult;
        using util::Error;

        std::lock_guard lock(mtx);

        auto res = query_by_type_locked(type);

        if (res.empty())
        {
            return Ret::Err(
                Error::state()
                    .target_not_found() // 没有找到指定 Type 的事件
                    .message("No events found for specified Type")
                    // .context(to_string(type))
                    .build());
        }

        return Ret::Ok(res.back());
    }

    Timeline::EvList
    Timeline::query_by_fd_locked(int fd) const
    {
        EvList result;
        auto it = fd_index.find(fd);
        if (it == fd_index.end())
            return result;

        result.reserve(it->second.size());
        for (auto idx : it->second)
            if (idx < events.size())
                result.push_back(events[idx]);

        return result;
    }

    Timeline::EvList
    Timeline::query_by_type_locked(
        EventType type) const
    {
        EvList result;
        auto it = type_index.find(type);
        if (it == type_index.end())
            return result;

        result.reserve(it->second.size());
        for (auto idx : (*it).second)
            if (idx < events.size())
                result.push_back(events[idx]);

        return result;
    }

    Timeline::EvList
    Timeline::query_by_time_locked(
        TimeStamp start,
        TimeStamp end) const
    {
        EvList result;
        if (start > end)
            return result;

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
            result.push_back(*it);

        return result;
    }

    void Timeline::rebuild_indexes_locked(const std::vector<Event> &arr)
    {
        fd_index.clear();
        type_index.clear();

        size_t len = arr.size();
        for (size_t idx = 0; idx < len; ++idx)
        {
            auto &e = arr[idx];
            if (e.fd)
                fd_index[e.fd.fd].push_back(idx);

            type_index[e.type].push_back(idx);
        }
    }

    Timeline::EvCnt
    Timeline::remove_by_fd_locked(platform::fd::FdView fd)
    {
        return remove_if_locked(
            [fd](const Event &e)
            { return e.fd == fd; });
    }

    Timeline::EvCnt
    Timeline::remove_by_type_locked(EventType type)
    {
        return remove_if_locked(
            [type](const Event &e)
            { return e.type == type; });
    }

    Timeline::EvCnt
    Timeline::remove_by_time_locked(
        TimeStamp start,
        TimeStamp end)
    {
        return remove_if_locked(
            [start, end](const Event &e)
            { return start <= e.ts && e.ts < end; });
    }
}