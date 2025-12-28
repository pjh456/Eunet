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