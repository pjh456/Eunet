#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "eunet/platform/socket/tcp_socket.hpp"
#include "eunet/platform/address.hpp"

using namespace platform::net;
using namespace std::chrono_literals;

// 简单 TCP 回环 echo server，用于单元测试
void echo_server(uint16_t port, bool &running)
{
    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(server_fd >= 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    addr.sin_port = htons(port);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int ret = ::bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    assert(ret == 0);

    ret = ::listen(server_fd, 1);
    assert(ret == 0);

    running = true;

    int client_fd = ::accept(server_fd, nullptr, nullptr);
    if (client_fd >= 0)
    {
        std::vector<char> buf(1024);
        while (true)
        {
            ssize_t n = ::recv(client_fd, buf.data(), buf.size(), 0);
            if (n <= 0)
                break;
            ::send(client_fd, buf.data(), n, 0); // echo 回去
        }
        ::close(client_fd);
    }

    ::close(server_fd);
    running = false;
}

void test_tcpsocket()
{
    std::cout << "[TEST] TCPSocket unit test start\n";

    uint16_t port = 54321;
    bool server_running = false;

    // 启动本地 echo server
    std::thread server_thread(echo_server, port, std::ref(server_running));

    // 等待 server 启动
    while (!server_running)
        std::this_thread::sleep_for(10ms);

    // 1. 创建 socket
    auto sock_res = TCPSocket::create();
    assert(sock_res.is_ok() && "Failed to create TCPSocket");
    auto socket = std::move(sock_res.unwrap());

    // 2. 解析回环地址
    auto addrs_res = SocketAddress::resolve("127.0.0.1", port);
    assert(addrs_res.is_ok() && "Failed to resolve address");
    auto addrs = addrs_res.unwrap();
    assert(!addrs.empty() && "No addresses found");

    // 3. connect
    auto conn_res = socket.connect(addrs[0], 1s);
    assert(conn_res.is_ok() && "Connect failed");

    // 4. send/recv
    const char *msg = "hello world";
    auto send_res = socket.send_all(reinterpret_cast<const std::byte *>(msg), strlen(msg), 1s);
    assert(send_res.is_ok() && send_res.unwrap() == strlen(msg));

    std::vector<std::byte> buf(128);
    auto recv_res = socket.recv_some(buf.data(), send_res.unwrap(), 1s);
    assert(recv_res.is_ok() && recv_res.unwrap() == send_res.unwrap());

    std::string received(reinterpret_cast<const char *>(buf.data()), recv_res.unwrap());
    assert(received == msg);

    std::cout << "[INFO] Sent and received: " << received << "\n";

    socket.close();

    // 等待 server 结束
    server_thread.join();

    std::cout << "[TEST] TCPSocket unit test end\n";
}

int main()
{
    test_tcpsocket();
    return 0;
}
