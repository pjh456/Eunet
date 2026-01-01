#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>

#include <arpa/inet.h>
#include <unistd.h>

#include "eunet/platform/socket/udp_socket.hpp"
#include "eunet/platform/io_context.hpp"
#include "eunet/platform/net/endpoint.hpp"
#include "eunet/util/byte_buffer.hpp"

using namespace platform;
using namespace platform::net;
using namespace std::chrono_literals;

static void run_udp_echo_server(uint16_t port)
{
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    assert(fd >= 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);

    assert(::bind(fd, (sockaddr *)&addr, sizeof(addr)) == 0);

    std::byte buf[1024];
    sockaddr_in peer{};
    socklen_t peer_len = sizeof(peer);

    ssize_t n =
        ::recvfrom(
            fd,
            buf,
            sizeof(buf),
            0,
            (sockaddr *)&peer,
            &peer_len);

    assert(n > 0);

    ssize_t m =
        ::sendto(
            fd,
            buf,
            n,
            0,
            (sockaddr *)&peer,
            peer_len);

    assert(m == n);

    ::close(fd);
}

void test_io_context_udp_read_write()
{
    constexpr uint16_t PORT = 23457;

    std::thread server([&]
                       { run_udp_echo_server(PORT); });

    std::this_thread::sleep_for(50ms);

    /* ---------- IOContext ---------- */
    auto ctx_res = IOContext::create();
    assert(ctx_res.is_ok());
    IOContext ctx = std::move(ctx_res.unwrap());

    /* ---------- socket ---------- */
    auto sock_res = UDPSocket::create(AddressFamily::IPv4);
    assert(sock_res.is_ok());
    UDPSocket sock = std::move(sock_res.unwrap());

    auto ep_res = Endpoint::from_string("127.0.0.1", PORT);
    assert(ep_res.is_ok());
    Endpoint ep = ep_res.unwrap();

    /* UDP connect：只是设置默认 peer */
    assert(sock.try_connect(ep).is_ok());

    /* ---------- write ---------- */
    util::ByteBuffer write_buf(128);
    const char *msg = "hello udp io_context";

    {
        auto span = write_buf.prepare(std::strlen(msg));
        std::memcpy(span.data(), msg, span.size());
        write_buf.commit(span.size());
    }

    auto write_res = ctx.write(sock, write_buf, 1s);
    assert(write_res.is_ok());
    auto write_len = write_res.unwrap();
    assert(write_len == std::strlen(msg));
    assert(write_buf.empty());

    /* ---------- read ---------- */
    util::ByteBuffer read_buf(128);

    auto read_res = ctx.read(sock, read_buf, 2s);
    // if (read_res.is_err())
    // {
    //     std::cout << read_res.unwrap_err().format() << std::endl;
    // }
    assert(read_res.is_ok());
    auto read_len = read_res.unwrap();
    assert(read_len == std::strlen(msg));

    auto readable = read_buf.readable();
    std::string echoed(
        reinterpret_cast<const char *>(readable.data()),
        readable.size());

    assert(echoed == msg);

    read_buf.consume(readable.size());
    assert(read_buf.empty());

    server.join();

    std::cout << "[OK] test_io_context_udp_read_write\n";
}

int main()
{
    test_io_context_udp_read_write();
    return 0;
}