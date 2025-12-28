#include <thread>
#include <string>
#include <chrono>

#include "eunet/core/orchestrator.hpp"
#include "eunet/net/async_tcp_client.hpp"
#include "eunet/tui/tui_app.hpp"

int main()
{
    core::Orchestrator orch;
    ui::TuiApp app(orch);

    // 1. 在后台线程跑网络请求
    std::thread net_thread(
        [&orch]()
        {
            net::tcp::AsyncTCPClient client(orch);

            // 模拟一个 HTTP 请求流程
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 给 UI 启动时间
            (void)client.connect("www.baidu.com", 80);

            std::string req = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n";
            std::vector<std::byte> data;
            for (char c : req)
                data.push_back(static_cast<std::byte>(c));

            (void)client.send(data);

            std::vector<std::byte> buf;
            (void)client.recv(buf, 1024);
            (void)client.close();
        });

    // 2. 主线程跑 TUI 循环
    app.run();

    if (net_thread.joinable())
        net_thread.join();
    return 0;
}