#include <thread>
#include <string>
#include <chrono>

#include "eunet/core/orchestrator.hpp"
#include "eunet/net/async_tcp_client.hpp"
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