/*
 * ============================================================================
 *  File Name   : scenario.hpp
 *  Module      : core
 *
 *  Description :
 *      网络场景的抽象基类。定义了 `run` 接口，任何具体的网络任务
 *      （如 HTTP 请求、Ping 探测）都继承此类并在 run 中执行逻辑。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_CORE_SCENARIO
#define INCLUDE_EUNET_CORE_SCENARIO

#include "eunet/util/result.hpp"
#include "eunet/core/orchestrator.hpp"

namespace core::scenario
{
    class Scenario
    {
    public:
        using RunResult = util::ResultV<void>;

    public:
        virtual ~Scenario() = default;
        virtual RunResult run(Orchestrator &orch) = 0;
    };
}

#endif // INCLUDE_EUNET_CORE_SCENARIO