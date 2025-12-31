// #include <cassert>
// #include <thread>
// #include <vector>
// #include <string>
// #include <iostream>

// #include "eunet/platform/socket/udp_socket.hpp"
// #include "eunet/net/connection/udp_connection.hpp"

// using namespace platform::net;
// using namespace net::udp;

// // 辅助函数：将字符串转为 std::byte 向量
// std::vector<std::byte> to_bytes(const std::string &str)
// {
//     std::vector<std::byte> bytes;
//     for (char c : str)
//         bytes.push_back(static_cast<std::byte>(c));
//     return bytes;
// }

// // 辅助函数：将 std::byte 数组转回字符串
// std::string to_string(const std::byte *data, size_t len)
// {
//     return std::string(reinterpret_cast<const char *>(data), len);
// }

// // 1. 测试 Socket 创建
// void test_create_socket()
// {
//     auto res = UDPSocket::create(AddressFamily::IPv4);
//     assert(res.is_ok());

//     auto sock = std::move(res.unwrap());
//     assert(sock.is_open());
// }

// // 2. 测试地址解析与获取
// void test_local_address_after_connect()
// {
//     auto res = UDPSocket::create(AddressFamily::IPv4);
//     assert(res.is_ok());

//     auto sock = std::move(res.unwrap());

//     // 连接到环回地址的一个随机端口
//     auto addr = SocketAddress::loopback_ipv4(12345);
//     auto conn_res = sock.connect(addr);
//     assert(conn_res.is_ok());

//     auto local_res = sock.local_address();
//     assert(local_res.is_ok());

//     auto *sa =
//         reinterpret_cast<const sockaddr_in *>(
//             local_res.unwrap().as_sockaddr());

//     assert(ntohs(sa->sin_port) != 0);
// }

// // 3. 测试两个 Socket 之间的端到端通信
// void test_send_and_receive()
// {
//     auto s_res = UDPSocket::create(AddressFamily::IPv4);
//     auto c_res = UDPSocket::create(AddressFamily::IPv4);

//     assert(s_res.is_ok());
//     assert(c_res.is_ok());

//     auto server_sock = std::move(s_res.unwrap());
//     auto client_sock = std::move(c_res.unwrap());

//     // 先 connect 到 0 端口，让 OS 分配端口
//     auto loopback = SocketAddress::loopback_ipv4(0);

//     server_sock.connect(loopback).unwrap();
//     auto server_addr = server_sock.local_address().unwrap();

//     client_sock.connect(loopback).unwrap();
//     auto client_addr = client_sock.local_address().unwrap();

//     // Cross-connect
//     server_sock.connect(client_addr).unwrap();
//     client_sock.connect(server_addr).unwrap();

//     // 发送
//     std::string msg = "Hello UDP";
//     auto send_data = to_bytes(msg);

//     auto send_res =
//         client_sock.send(send_data.data(), send_data.size());

//     assert(send_res.is_ok());
//     assert(send_res.unwrap() == msg.size());

//     // 接收
//     std::byte buf[1024];
//     auto recv_res =
//         server_sock.recv(buf, sizeof(buf));

//     assert(recv_res.is_ok());
//     assert(recv_res.unwrap() == msg.size());
//     assert(to_string(buf, recv_res.unwrap()) == msg);
// }

// // 4. 测试 UDPConnection 包装器
// void test_connection_wrapper()
// {
//     auto peer_addr = SocketAddress::loopback_ipv4(9999);

//     auto conn_res = UDPConnection::connect(peer_addr);
//     assert(conn_res.is_ok());

//     auto conn = std::move(conn_res.unwrap());
//     assert(conn.is_open());

//     // 非阻塞模式
//     platform::net::NonBlockingGuard nb_guard(conn.fd());

//     std::byte buf[10];
//     auto read_res =
//         conn.read(buf, sizeof(buf), platform::time::Duration(0));

//     // 根据当前实现：EAGAIN => Ok(0)
//     assert(read_res.is_ok());
//     assert(read_res.unwrap() == 0);
// }

// // 5. 测试关闭后操作
// void test_operate_on_closed_socket()
// {
//     auto res = UDPSocket::create(AddressFamily::IPv4);
//     assert(res.is_ok());

//     auto sock = std::move(res.unwrap());
//     sock.close();

//     std::string msg = "fail";
//     auto data = to_bytes(msg);

//     auto send_res =
//         sock.send(data.data(), data.size());

//     assert(send_res.is_err());
// }

// int main()
// {
//     test_create_socket();
//     test_local_address_after_connect();
//     test_send_and_receive();
//     test_connection_wrapper();
//     test_operate_on_closed_socket();

//     std::cout << "[UDPSocket / UDPConnection] all tests passed.\n";
//     return 0;
// }

int main()
{
    return 0;
}