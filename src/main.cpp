/*
 * ============================================================================
 *  File Name   : main.cpp
 *  Module      : root
 *
 *  Description :
 *      程序主入口。解析命令行参数，初始化 Orchestrator、Engine 和 TUI App，
 *      组装各模块并启动主循环。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#include <thread>
#include <string>
#include <chrono>

#include "eunet/core/orchestrator.hpp"
#include "eunet/tui/tui_app.hpp"
#include "eunet/net/http_scenario.hpp"

int main(int argc, char **argv)
{
    std::string target_url = "http://www.baidu.com"; // 默认值

    if (argc > 1)
    {
        target_url = argv[1]; // 接受命令行第一个参数作为 URL
    }

    core::Orchestrator orch;
    core::NetworkEngine engine(orch);
    ui::TuiApp app(orch, engine); // 把引擎传给 UI

    engine.execute(std::make_unique<net::http::HttpGetScenario>(target_url));

    app.run();
    return 0;
}