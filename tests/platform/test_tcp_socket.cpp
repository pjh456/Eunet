#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstring>
#include <arpa/inet.h>

#include "eunet/platform/tcp_socket.hpp"
#include "eunet/platform/address.hpp"

using namespace platform::net;
using namespace std::chrono_literals;

void test_tcpsocket()
{
    std::cout << "[TEST] TCPSocket unit test start\n";

    // 1. 创建 socket
    auto sock_res = TCPSocket::create();
    assert(sock_res.is_ok() && "Failed to create TCPSocket");
    auto socket = std::move(sock_res.unwrap());

    // 2. 解析本地回环地址 (假设本地启动 echo 服务器)
    auto addrs_res = SocketAddress::resolve("127.0.0.1", 12345);
    assert(addrs_res.is_ok() && "Failed to resolve address");
    auto addrs = addrs_res.unwrap();
    assert(!addrs.empty() && "No addresses found");

    // 3. 测试 connect 超时（假设没有服务监听）
    auto timeout = platform::time::Duration(500ms);
    auto conn_res = socket.connect(addrs[0], timeout);
    if (conn_res.is_err())
    {
        std::cout << "[INFO] Connect expectedly failed (no server): "
                  << conn_res.unwrap_err().message() << "\n";
    }
    else
    {
        std::cout << "[INFO] Connect succeeded\n";

        // 4. 测试 send/recv
        const char *msg = "hello";
        auto send_res = socket.send(reinterpret_cast<const std::byte *>(msg), strlen(msg), 1s);
        assert(send_res.is_ok() && "Send failed");
        std::cout << "[INFO] Sent " << send_res.unwrap() << " bytes\n";

        std::vector<std::byte> buf(128);
        auto recv_res = socket.recv(buf.data(), 128, 1s);
        if (recv_res.is_ok())
        {
            std::cout << "[INFO] Received " << recv_res.unwrap() << " bytes: ";
            std::string s(reinterpret_cast<char *>(buf.data()), recv_res.unwrap());
            std::cout << s << "\n";
        }
        else
        {
            std::cout << "[INFO] Recv failed: "
                      << recv_res.unwrap_err().message() << "\n";
        }
    }

    socket.close();
    std::cout << "[TEST] TCPSocket unit test end\n";
}

int main()
{
    test_tcpsocket();
    return 0;
}
