#include <cassert>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include "eunet/platform/tcp_socket.hpp"
#include "eunet/net/connection/tcp_connection.hpp"

#include <iostream>

void test_tcp_connection()
{
    using namespace net::tcp;
    using namespace platform::net;
    using namespace std::chrono;

    // 1. 启动本地服务器
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd >= 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; // 让系统分配端口

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    assert(::bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0);
    assert(::listen(listen_fd, 1) == 0);

    socklen_t addrlen = sizeof(addr);
    assert(::getsockname(listen_fd, reinterpret_cast<sockaddr *>(&addr), &addrlen) == 0);

    uint16_t port = ntohs(addr.sin_port);

    // 2. 启动客户端连接线程
    std::thread client_thread(
        [port]()
        {
            auto sock_res = TCPSocket::create();
            assert(sock_res.is_ok());
            TCPSocket sock = std::move(sock_res.unwrap());

            SocketAddress server_addr({AF_INET, htons(port), {htonl(INADDR_LOOPBACK)}});

            auto res = sock.connect(server_addr, milliseconds(500));
            assert(res.is_ok());

            auto conn = TCPConnection::from_accepted_socket(std::move(sock));

            // 发送数据
            std::vector<std::byte> data = {std::byte(1), std::byte(2), std::byte(3)};
            auto send_res = conn.write(data.data(), data.size(), milliseconds(500));
            assert(send_res.is_ok());
            (void)conn.flush();

            conn.close();
        });

    // 3. 接收端
    int client_fd = ::accept(listen_fd, nullptr, nullptr);
    assert(client_fd >= 0);

    std::vector<uint8_t> buf(16);
    ssize_t n = ::recv(client_fd, buf.data(), buf.size(), 0);
    assert(n == 3);
    assert(buf[0] == 1 && buf[1] == 2 && buf[2] == 3);

    ::close(client_fd);
    ::close(listen_fd);

    client_thread.join();

    std::cout << "TCPConnection tests passed.\n";
}

int main()
{
    test_tcp_connection();
    return 0;
}
