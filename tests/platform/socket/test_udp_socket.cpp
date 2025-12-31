// #include <cassert>
// #include <cstring>
// #include <thread>

// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <unistd.h>

// #include "eunet/platform/socket/udp_socket.hpp"
// #include "eunet/platform/address.hpp"

// using platform::net::SocketAddress;
// using platform::net::UDPSocket;
// using platform::time::Duration;

// namespace
// {
//     constexpr uint16_t TEST_PORT = 34567;
// }

// /*
//  * 简单 UDP Echo Server（阻塞）
//  */
// static void udp_echo_server()
// {
//     int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
//     assert(fd >= 0);

//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
//     addr.sin_port = htons(TEST_PORT);

//     int ret = ::bind(
//         fd,
//         reinterpret_cast<sockaddr *>(&addr),
//         sizeof(addr));
//     assert(ret == 0);

//     std::byte buf[1024];
//     sockaddr_in peer{};
//     socklen_t peer_len = sizeof(peer);

//     ssize_t n = ::recvfrom(
//         fd,
//         buf,
//         sizeof(buf),
//         0,
//         reinterpret_cast<sockaddr *>(&peer),
//         &peer_len);

//     assert(n > 0);

//     // echo 回去
//     ssize_t sent = ::sendto(
//         fd,
//         buf,
//         static_cast<size_t>(n),
//         0,
//         reinterpret_cast<sockaddr *>(&peer),
//         peer_len);

//     assert(sent == n);

//     ::close(fd);
// }

// int main()
// {
//     // 启动 echo server
//     std::thread server_thread(udp_echo_server);

//     // 给 server 一点时间 bind
//     std::this_thread::sleep_for(std::chrono::milliseconds(50));

//     // 创建 UDPSocket
//     auto sock_res = UDPSocket::create();
//     assert(sock_res.is_ok());

//     auto sock = std::move(sock_res.unwrap());

//     auto peer_res = SocketAddress::resolve("127.0.0.1", TEST_PORT);
//     assert(peer_res.is_ok());

//     auto conn_res = sock.connect(peer_res.unwrap()[0]);
//     assert(conn_res.is_ok());

//     const char *msg = "hello udp";
//     size_t msg_len = std::strlen(msg);

//     // send
//     auto send_res = sock.send(
//         reinterpret_cast<const std::byte *>(msg),
//         msg_len);
//     assert(send_res.is_ok());
//     assert(send_res.unwrap() == msg_len);

//     // recv
//     std::byte buf[128];
//     auto recv_res = sock.recv(
//         buf,
//         sizeof(buf));
//     assert(recv_res.is_ok());
//     assert(recv_res.unwrap() == msg_len);

//     assert(
//         std::memcmp(
//             buf,
//             msg,
//             msg_len) == 0);

//     sock.close();

//     server_thread.join();
//     return 0;
// }
int main()
{
    return 0;
}