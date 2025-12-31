// #include "eunet/net/connection/udp_connection.hpp"

// namespace net::udp
// {
//     util::ResultV<UDPConnection>
//     UDPConnection::connect(
//         const platform::net::SocketAddress &peer)
//     {
//         using Result = util::ResultV<UDPConnection>;

//         auto sock_res = platform::net::UDPSocket::create();
//         if (sock_res.is_err())
//             return Result::Err(sock_res.unwrap_err());

//         auto &sock = sock_res.unwrap();
//         auto res = sock.connect(peer);
//         if (res.is_err())
//             return Result::Err(res.unwrap_err());

//         return Result::Ok(
//             UDPConnection(std::move(sock)));
//     }

//     UDPConnection
//     UDPConnection::from_socket(
//         platform::net::UDPSocket &&sock)
//     {
//         return UDPConnection(std::move(sock));
//     }

//     UDPConnection::UDPConnection(
//         platform::net::UDPSocket &&sock)
//         : sock(std::move(sock)) {}

//     platform::fd::FdView
//     UDPConnection::fd()
//         const noexcept { return sock.view(); }

//     void UDPConnection::close() noexcept { sock.close(); }

//     bool UDPConnection::is_open()
//         const noexcept { return sock.view().fd >= 0; }

//     util::ResultV<size_t>
//     UDPConnection::write(
//         const std::byte *buf,
//         size_t len,
//         platform::time::Duration timeout)
//     {
//         // UDP：一次 write = 一个 datagram
//         auto res = sock.send(buf, len);
//         if (res.is_err())
//         {
//             auto err = res.unwrap_err();
//             if (err.code() == EAGAIN ||
//                 err.code() == EWOULDBLOCK)
//                 return util::ResultV<size_t>::Ok(0);
//             return util::ResultV<size_t>::Err(err);
//         }

//         // UDP 理论上不应发生 partial send
//         return util::ResultV<size_t>::Ok(res.unwrap());
//     }

//     util::ResultV<size_t>
//     UDPConnection::read(
//         std::byte *buf,
//         size_t len,
//         platform::time::Duration timeout)
//     {
//         // UDP：一次 read = 一个 datagram（最多 len）
//         auto res = sock.recv(buf, len);
//         if (res.is_err())
//         {
//             if (res.unwrap_err().code() == EAGAIN ||
//                 res.unwrap_err().code() == EWOULDBLOCK)
//                 return util::ResultV<size_t>::Ok(0);

//             return util::ResultV<size_t>::Err(res.unwrap_err());
//         }

//         return util::ResultV<size_t>::Ok(res.unwrap());
//     }
// }
