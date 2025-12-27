#ifndef INCLUDE_EUNET_CORE_EVENT_SNAP_SHOOT
#define INCLUDE_EUNET_CORE_EVENT_SNAP_SHOOT

#include "eunet/core/event.hpp"
#include "eunet/core/lifecycle_fsm.hpp"

namespace core
{
    struct EventSnapshot
    {
        const Event *event; // 发生的事件（只读）
        int fd;

        LifeState state;              // FSM 当前状态
        platform::time::WallPoint ts; // 事件时间

        bool has_error;
        const EventError *error; // 可空
    };

}

#endif // INCLUDE_EUNET_CORE_EVENT_SNAP_SHOOT