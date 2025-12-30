#ifndef INCLUDE_EUNET_CORE_ENGINE
#define INCLUDE_EUNET_CORE_ENGINE

#include <thread>
#include <memory>
#include <atomic>

#include "eunet/core/scenario.hpp"

namespace core
{
    class NetworkEngine
    {
    private:
        Orchestrator &orch_;
        std::unique_ptr<std::thread> worker_;
        std::atomic<bool> running_{false};

    public:
        explicit NetworkEngine(Orchestrator &orch) : orch_(orch) {}

        ~NetworkEngine()
        {
            if (worker_ && worker_->joinable())
            {
                // 如果引擎销毁时线程还在跑，应在此处考虑停止逻辑
                // 生产环境建议通过 stop_token 通知 scenario 退出
                worker_->join();
            }
        }

    public:
        void execute(std::unique_ptr<scenario::Scenario> scenario)
        {
            // 确保同一时间只有一个 Scenario 在运行（如果需要多并发，可改用线程池）
            bool expected = false;
            if (!running_.compare_exchange_strong(expected, true))
            {
                return;
            }

            worker_ = std::make_unique<std::thread>(
                [this, sc = std::move(scenario)]()
                {
                    try
                    {
                        (void)sc->run(orch_);
                    }
                    catch (...)
                    {
                        // 防止线程内异常导致程序崩溃
                    }
                    running_.store(false);
                });
        }

        bool is_running() const { return running_; }
    };
}

#endif // INCLUDE_EUNET_CORE_ENGINE