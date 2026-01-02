#include <cassert>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include "eunet/platform/socket/tcp_socket.hpp"
#include "eunet/net/connection/tcp_connection.hpp"

#include <iostream>

void test_tcp_connection()
{
    using namespace net::tcp;
    using namespace platform::net;
    using namespace std::chrono;

    // ---------- server ----------
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd >= 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    assert(::bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) == 0);
    assert(::listen(listen_fd, 1) == 0);

    socklen_t len = sizeof(addr);
    assert(::getsockname(listen_fd, (sockaddr *)&addr, &len) == 0);
    uint16_t port = ntohs(addr.sin_port);

    // ---------- client ----------
    std::thread client(
        [port]()
        {
            auto addr =
                Endpoint::from_ipv4(
                    htonl(INADDR_LOOPBACK), port);

            auto conn_res =
                TCPConnection::connect(addr, 500);
            assert(conn_res.is_ok());

            auto conn = std::move(conn_res.unwrap());

            std::vector<std::byte> data{
                std::byte{1}, std::byte{2}, std::byte{3}};

            util::ByteBuffer buf;
            buf.append(data);

            auto wr = conn.write(
                buf, 500);
            assert(wr.is_ok());

            auto flush_res = conn.flush();
            conn.close();
        });

    // ---------- accept ----------
    int client_fd = ::accept(listen_fd, nullptr, nullptr);
    assert(client_fd >= 0);

    std::vector<uint8_t> buf(16);
    ssize_t n = ::recv(client_fd, buf.data(), buf.size(), 0);
    assert(n == 3);
    assert(buf[0] == 1 && buf[1] == 2 && buf[2] == 3);

    ::close(client_fd);
    ::close(listen_fd);

    client.join();

    std::cout << "TCPConnection test passed.\n";
}

int main()
{
    test_tcp_connection();
    return 0;
}