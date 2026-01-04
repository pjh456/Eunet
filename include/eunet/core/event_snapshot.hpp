/*
 * ============================================================================
 *  File Name   : event_snapshot.hpp
 *  Module      : core
 *
 *  Description :
 *      事件快照定义。这是用于 UI 展示的数据结构，聚合了 Event 本身、
 *      当时的生命周期状态 (LifeState) 以及可能的错误，设计为只读/拷贝安全。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_CORE_EVENT_SNAPSHOT
#define INCLUDE_EUNET_CORE_EVENT_SNAPSHOT

#include <optional>

#include "eunet/util/error.hpp"
#include "eunet/core/event.hpp"
#include "eunet/core/lifecycle_fsm.hpp"

namespace core
{
    struct EventSnapshot
    {
        Event event;
        int fd;
        LifeState state;
        platform::time::WallPoint ts;
        std::optional<util::Error> error = std::nullopt;
    };

}

#endif // INCLUDE_EUNET_CORE_EVENT_SNAPSHOT