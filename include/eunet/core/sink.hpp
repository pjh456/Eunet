/*
 * ============================================================================
 *  File Name   : sink.hpp
 *  Module      : core
 *
 *  Description :
 *      事件接收器 (Sink) 接口定义。遵循观察者模式，允许不同的模块
 *      （如 TUI、日志记录器）订阅 Orchestrator 产生的事件快照。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_CORE_SINK
#define INCLUDE_EUNET_CORE_SINK

#include "eunet/core/event_snapshot.hpp"

namespace core::sink
{
    class IEventSink
    {
    public:
        virtual ~IEventSink() = default;

        virtual void on_event(const EventSnapshot &snap) = 0;
    };

}

#endif // INCLUDE_EUNET_CORE_SINK