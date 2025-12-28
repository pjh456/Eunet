#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstddef>

#include "eunet/net/sync_tcp_client.hpp"
#include "eunet/core/event.hpp"
#include "eunet/core/orchestrator.hpp"

// 简单打印 Timeline
void print_timeline(const core::Timeline &tl)
{
    for (const auto &e : tl.replay_all())
    {
        std::cout
            << "Event: " << static_cast<int>(e->type)
            << " | " << to_string(*e) << "\n";
    }
}

int main()
{
    core::Orchestrator orch;
    net::tcp::SyncTCPClient client(orch);

    // 连接 example.com:80
    auto res = client.connect("example.com", 80);
    if (res.is_err())
    {
        std::cerr << "Connect failed: " << res.unwrap_err().message() << "\n";
        return 1;
    }

    // 发送 HTTP GET 请求
    std::string http_request = "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n";
    std::vector<std::byte> data(http_request.size());
    std::memcpy(data.data(), http_request.data(), http_request.size());

    auto send_res = client.send(data);
    if (send_res.is_err())
    {
        std::cerr << "Send failed: " << send_res.unwrap_err().message() << "\n";
        return 1;
    }
    std::cout << "Sent " << send_res.unwrap() << " bytes\n";

    // 接收响应
    std::vector<std::byte> buffer;
    auto recv_res = client.recv(buffer, 4096); // 读最多 4KB
    if (recv_res.is_err())
    {
        std::cerr << "Recv failed: " << recv_res.unwrap_err().message() << "\n";
        return 1;
    }

    std::cout << "Received " << recv_res.unwrap() << " bytes\n";
    std::string resp(reinterpret_cast<const char *>(buffer.data()), buffer.size());
    std::cout << "--- Response Start ---\n";
    std::cout << resp << "\n";
    std::cout << "--- Response End ---\n";

    client.close();

    // 打印事件
    print_timeline(orch.get_timeline());

    return 0;
}
