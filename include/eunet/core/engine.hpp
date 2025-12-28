#ifndef INCLUDE_EUNET_CORE_ENGINE
#define INCLUDE_EUNET_CORE_ENGINE

#include <thread>
#include <memory>

#include "eunet/core/scenario.hpp"

namespace core
{
    class NetworkEngine
    {
    private:
        Orchestrator &orch_;
        std::unique_ptr<std::thread> worker_;
        bool running_ = false;

    public:
        explicit NetworkEngine(Orchestrator &orch) : orch_(orch) {}

        void execute(std::unique_ptr<scenario::Scenario> scenario)
        {
            if (running_)
                return;
            running_ = true;

            worker_ = std::make_unique<std::thread>(
                [this, sc = std::move(scenario)]()
                {
                    (void)sc->run(orch_);
                    running_ = false;
                });
            worker_->detach(); // 生产环境建议用更稳妥的线程管理
        }

        bool is_running() const { return running_; }
    };
}

#endif // INCLUDE_EUNET_CORE_ENGINE