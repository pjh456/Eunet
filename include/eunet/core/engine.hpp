/*
 * ============================================================================
 *  File Name   : engine.hpp
 *  Module      : core
 *
 *  Description :
 *      执行引擎。负责在独立的后台线程中运行 Scenario，确保网络阻塞操作
 *      不会卡住 UI 线程。管理 Worker 线程的生命周期。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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
                worker_->join();
            }
        }

    public:
        bool execute(std::unique_ptr<scenario::Scenario> scenario)
        {
            // 确保同一时间只有一个 Scenario 在运行（如果需要多并发，可改用线程池）
            bool expected = false;
            if (!running_.compare_exchange_strong(expected, true))
            {
                return false; // 引擎忙碌中，忽略本次请求
            }

            // 在创建新线程前，回收旧线程资源
            if (worker_)
            {
                if (worker_->joinable())
                {
                    // 因为 running_ 已经是 false，这里的 join 会立即返回
                    worker_->join();
                }
                worker_.reset(); // 释放旧指针
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

            return true;
        }

        bool is_running() const { return running_; }
    };
}

#endif // INCLUDE_EUNET_CORE_ENGINE