#include <cassert>
#include <thread>
#include <string>
#include <iostream>

#include "eunet/util/byte_buffer.hpp"
#include "eunet/platform/socket/udp_socket.hpp"
#include "eunet/net/connection/udp_connection.hpp"

using namespace platform::net;
using namespace net::udp;

// --------------------------------------------------
// 工具函数
// --------------------------------------------------

static util::ByteBuffer
make_buffer(const std::string &s)
{
    util::ByteBuffer buf(s.size());
    buf.append(
        std::span<const std::byte>(
            reinterpret_cast<const std::byte *>(s.data()),
            s.size()));
    return buf;
}

static std::string
buffer_to_string(util::ByteBuffer &buf)
{
    auto r = buf.readable();
    std::string s(
        reinterpret_cast<const char *>(r.data()),
        r.size());
    buf.consume(r.size());
    return s;
}

// --------------------------------------------------
// 1. 创建 Socket
// --------------------------------------------------

void test_create_socket()
{
    auto res = UDPSocket::create(AddressFamily::IPv4);
    assert(res.is_ok());

    auto sock = std::move(res.unwrap());
    assert(sock.is_open());
}

// --------------------------------------------------
// 2. connect 后应有本地端口
// --------------------------------------------------

void test_local_address_after_connect()
{
    auto res = UDPSocket::create(AddressFamily::IPv4);
    assert(res.is_ok());

    auto sock = std::move(res.unwrap());

    auto ep = Endpoint::loopback_ipv4(12345);
    assert(sock.connect(ep).is_ok());

    auto local = sock.local_endpoint();
    assert(local.is_ok());

    auto *sa =
        reinterpret_cast<const sockaddr_in *>(
            local.unwrap().as_sockaddr());

    assert(ntohs(sa->sin_port) != 0);
}

// --------------------------------------------------
// 3. UDPSocket 端到端通信（ByteBuffer 版）
// --------------------------------------------------

void test_socket_send_and_receive()
{
    auto s_res = UDPSocket::create(AddressFamily::IPv4);
    auto c_res = UDPSocket::create(AddressFamily::IPv4);
    assert(s_res.is_ok());
    assert(c_res.is_ok());

    auto server = std::move(s_res.unwrap());
    auto client = std::move(c_res.unwrap());

    // 绑定随机端口
    server.connect(Endpoint::loopback_ipv4(0)).unwrap();
    client.connect(Endpoint::loopback_ipv4(0)).unwrap();

    auto server_ep = server.local_endpoint().unwrap();
    auto client_ep = client.local_endpoint().unwrap();

    // cross connect
    server.connect(client_ep).unwrap();
    client.connect(server_ep).unwrap();

    util::ByteBuffer send_buf = make_buffer("Hello UDP");
    util::ByteBuffer recv_buf(1024);

    auto wr = client.write(send_buf);
    assert(wr.is_ok());
    assert(wr.unwrap() == 9);

    auto rd = server.read(recv_buf);
    assert(rd.is_ok());
    assert(rd.unwrap() == 9);

    assert(buffer_to_string(recv_buf) == "Hello UDP");
}

// --------------------------------------------------
// 4. UDPConnection 封装测试
// --------------------------------------------------

void test_udp_connection()
{
    // server
    auto s = UDPConnection::connect(
                 Endpoint::loopback_ipv4(0))
                 .unwrap();
    auto server_ep = s.socket().local_endpoint().unwrap();

    // client
    auto c = UDPConnection::connect(server_ep).unwrap();

    util::ByteBuffer out = make_buffer("ping");
    util::ByteBuffer in(64);

    auto wr = c.write(out);
    assert(wr.is_ok());
    assert(wr.unwrap() == 4);

    auto rd = s.read(in);
    assert(rd.is_ok());
    assert(rd.unwrap() == 4);

    assert(buffer_to_string(in) == "ping");
}

// --------------------------------------------------
// 5. 关闭后操作
// --------------------------------------------------

void test_operate_on_closed_socket()
{
    auto res = UDPSocket::create(AddressFamily::IPv4);
    assert(res.is_ok());

    auto sock = std::move(res.unwrap());
    sock.close();

    util::ByteBuffer buf = make_buffer("fail");

    auto wr = sock.write(buf);
    assert(wr.is_err());
}

// --------------------------------------------------

int main()
{
    test_create_socket();
    test_local_address_after_connect();
    test_socket_send_and_receive();
    test_udp_connection();
    test_operate_on_closed_socket();

    std::cout << "[UDPSocket / UDPConnection] all tests passed.\n";
    return 0;
}
