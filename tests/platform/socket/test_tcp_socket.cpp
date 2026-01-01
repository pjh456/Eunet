#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>

#include <arpa/inet.h>
#include <unistd.h>

#include "eunet/platform/socket/tcp_socket.hpp"
#include "eunet/platform/net/endpoint.hpp"
#include "eunet/util/byte_buffer.hpp"

using namespace platform::net;
using namespace std::chrono_literals;

static void run_echo_server(uint16_t port)
{
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd >= 0);

    int opt = 1;
    ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);

    assert(::bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) == 0);
    assert(::listen(listen_fd, 1) == 0);

    int conn = ::accept(listen_fd, nullptr, nullptr);
    assert(conn >= 0);

    std::byte buf[1024];
    ssize_t n = ::recv(conn, buf, sizeof(buf), 0);
    assert(n > 0);

    ssize_t m = ::send(conn, buf, n, 0);
    assert(m == n);

    ::close(conn);
    ::close(listen_fd);
}

void test_tcp_blocking_read_write()
{
    constexpr uint16_t PORT = 23456;

    std::thread server([&]
                       { run_echo_server(PORT); });

    std::this_thread::sleep_for(50ms);

    /* ---------- socket ---------- */
    auto sock_res = TCPSocket::create(AddressFamily::IPv4);
    assert(sock_res.is_ok());
    TCPSocket sock = std::move(sock_res.unwrap());

    auto ep_res = Endpoint::from_string("127.0.0.1", PORT);
    assert(ep_res.is_ok());
    Endpoint ep = ep_res.unwrap();

    assert(sock.connect(ep).is_ok());

    /* ---------- write ---------- */
    util::ByteBuffer write_buf(128);
    const char *msg = "hello blocking tcp";

    {
        auto span = write_buf.prepare(std::strlen(msg));
        std::memcpy(span.data(), msg, span.size());
        write_buf.commit(span.size());
    }

    auto write_res = sock.write(write_buf);
    assert(write_res.is_ok());
    assert(write_res.unwrap() == std::strlen(msg));
    assert(write_buf.empty());

    /* ---------- read ---------- */
    util::ByteBuffer read_buf(128);

    auto read_res = sock.read(read_buf);
    assert(read_res.is_ok());
    assert(read_res.unwrap() == std::strlen(msg));

    auto readable = read_buf.readable();
    std::string echoed(
        reinterpret_cast<const char *>(readable.data()),
        readable.size());

    assert(echoed == msg);

    server.join();

    std::cout << "[OK] test_tcp_blocking_read_write\n";
}

int main()
{
    test_tcp_blocking_read_write();
    return 0;
}
