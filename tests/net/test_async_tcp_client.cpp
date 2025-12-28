#include <iostream>
#include <vector>
#include <string>

#include <fmt/format.h>

#include "eunet/core/orchestrator.hpp"
#include "eunet/core/sink.hpp"
#include "eunet/net/async_tcp_client.hpp"
#include "eunet/platform/time.hpp"

// 1. 实现一个简单的 ConsoleSink 用于观察事件流
class SimpleConsoleSink : public core::sink::IEventSink
{
public:
    void on_event(const core::EventSnapshot &snap) override
    {
        // 计算相对时间戳（毫秒）
        static auto start_time = platform::time::wall_now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            snap.ts - start_time)
                            .count();

        // 格式化输出：[时间][状态] 消息 (FD)
        std::string status_color = snap.has_error ? "\033[31m" : "\033[32m";
        std::string reset = "\033[0m";

        std::cout
            << fmt::format(
                   "[{:>4}ms] [{}{:<12}{}] {:<30} (fd: {})",
                   duration,
                   status_color, to_string(snap.state), reset,
                   snap.event->msg,
                   snap.fd)
            << std::endl;

        if (snap.has_error)
        {
            std::cout << "  └─ \033[31mError:\033[0m " << snap.error.format() << std::endl;
        }
    }
};

int main()
{
    using namespace net::tcp;
    using namespace std::chrono_literals;

    std::cout << "--- EuNet AsyncTCPClient Test ---" << std::endl;

    // 2. 初始化核心组件
    core::Orchestrator orch;
    SimpleConsoleSink console_sink;

    // 将观察者挂载到调度器
    orch.attach(&console_sink);

    // 3. 创建异步客户端
    AsyncTCPClient client(orch);

    // 4. 执行测试流程：连接 -> 发送 -> 接收
    const std::string host = "www.baidu.com";
    const uint16_t port = 80;

    std::cout << "Starting request to " << host << "..." << std::endl;

    // A. 连接测试
    auto conn_res = client.connect(host, port, 3000); // 3秒超时
    if (conn_res.is_err())
    {
        std::cerr << "Main logic aborted: Connection failed." << std::endl;
        return -1;
    }

    // B. 发送 HTTP GET 请求
    std::string raw_http =
        "GET / HTTP/1.1\r\n"
        "Host: " +
        host + "\r\n"
               "User-Agent: EuNet/0.1\r\n"
               "Connection: close\r\n\r\n";

    std::vector<std::byte> send_data;
    for (char c : raw_http)
        send_data.push_back(static_cast<std::byte>(c));

    auto send_res = client.send(send_data);
    if (send_res.is_err())
    {
        std::cerr << "Main logic aborted: Send failed." << std::endl;
        return -1;
    }

    // C. 接收数据测试
    std::vector<std::byte> recv_buf;
    auto recv_res = client.recv(recv_buf, 4096);
    if (recv_res.is_ok())
    {
        size_t bytes = recv_res.unwrap();
        std::cout << ">>> Received " << bytes << " bytes of data." << std::endl;

        // 打印前100个字符预览内容
        std::string preview;
        for (size_t i = 0; i < std::min(bytes, (size_t)100); ++i)
        {
            preview += static_cast<char>(recv_buf[i]);
        }
        std::cout << ">>> Preview: " << preview << "..." << std::endl;
    }

    // 5. 关闭连接
    client.close();

    std::cout << "--- Test Completed ---" << std::endl;

    // 最后可以打印一下 Timeline 里的总事件数
    std::cout << "Total events recorded in timeline: " << orch.get_timeline().size() << std::endl;

    return 0;
}