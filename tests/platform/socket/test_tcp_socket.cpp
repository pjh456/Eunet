#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "eunet/platform/net/endpoint.hpp"
#include "eunet/platform/socket/tcp_socket.hpp"
#include "eunet/util/byte_buffer.hpp"

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

    // 2. 构造回环地址 Endpoint
    auto ep = Endpoint::loopback_ipv4(port);

    // 3. connect
    auto conn_res = socket.connect(ep);
    assert(conn_res.is_ok() && "Connect failed");

    // 4. send
    const char *msg = "hello world";
    util::ByteBuffer send_buf(strlen(msg));
    send_buf.append(std::span<const std::byte>(reinterpret_cast<const std::byte *>(msg), strlen(msg)));
    auto write_res = socket.write(send_buf, 1s);
    assert(write_res.is_ok());
    assert(write_res.unwrap() == strlen(msg));

    // 5. recv
    util::ByteBuffer recv_buf(strlen(msg));
    auto read_res = socket.read(recv_buf, 1s);
    assert(read_res.is_ok());
    size_t n = read_res.unwrap();
    assert(n == strlen(msg));

    std::string received(reinterpret_cast<const char *>(recv_buf.readable().data()), n);
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
