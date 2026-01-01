// #include "eunet/platform/socket/udp_socket.hpp"
// #include "eunet/platform/poller.hpp"

// #include <sys/socket.h>
// #include <cerrno>

// namespace platform::net
// {

//     util::ResultV<UDPSocket>
//     UDPSocket::create(
//         AddressFamily af)
//     {
//         using Result = util::ResultV<UDPSocket>;

//         int domain =
//             (af == AddressFamily::IPv6)
//                 ? AF_INET6
//                 : AF_INET;

//         auto fd_res =
//             fd::Fd::socket(
//                 domain,
//                 SOCK_DGRAM,
//                 0);

//         if (fd_res.is_err())
//             return Result::Err(fd_res.unwrap_err());

//         return Result::Ok(
//             UDPSocket(std::move(fd_res.unwrap())));
//     }

//     IOResult
//     UDPSocket::try_read(
//         util::ByteBuffer &buf)
//     {
//         using Ret = IOResult;
//         using util::Error;

//         NonBlockingGuard guard(view());

//         auto buf_len = buf.writable_size();
//         auto buffer = buf.weak_prepare(buf_len);

//         ssize_t n =
//             ::recv(
//                 view().fd,
//                 buffer.data(),
//                 buf_len,
//                 0);

//         if (n > 0)
//         {
//             buf.weak_commit(n);
//             return Ret::Ok(static_cast<size_t>(n));
//         }
//         else if (n == 0)
//         {
//             // UDP 没有“对端关闭”的语义
//             return Ret::Ok(0);
//         }

//         int err_no = errno;
//         if (err_no == EAGAIN || err_no == EWOULDBLOCK)
//             return Ret::Ok(0);

//         return Ret::Err(
//             Error::transport()
//                 .code(err_no)
//                 .message("recv failed")
//                 .build());
//     }

//     IOResult
//     UDPSocket::try_write(
//         util::ByteBuffer &buf)
//     {
//         using Ret = IOResult;
//         using util::Error;

//         NonBlockingGuard guard(view());

//         auto data_len = buf.size();
//         auto data = buf.readable();

//         ssize_t n =
//             ::send(
//                 view().fd,
//                 data.data(),
//                 data_len,
//                 0);

//         if (n > 0)
//         {
//             buf.consume(n);
//             return Ret::Ok(static_cast<size_t>(n));
//         }
//         else if (n == 0)
//         {
//             // UDP send 返回 0 没有实际意义
//             return Ret::Ok(0);
//         }

//         int err_no = errno;
//         if (err_no == EAGAIN || err_no == EWOULDBLOCK)
//             return Ret::Ok(0);

//         return Ret::Err(
//             Error::transport()
//                 .code(err_no)
//                 .message("send failed")
//                 .build());
//     }

//     util::ResultV<void>
//     UDPSocket::try_connect(
//         const Endpoint &ep)
//     {
//         using Result = util::ResultV<void>;
//         using util::Error;

//         NonBlockingGuard guard(view());

//         int ret =
//             ::connect(
//                 view().fd,
//                 ep.as_sockaddr(),
//                 ep.length());

//         if (ret == 0)
//             return Result::Ok();

//         int err_no = errno;

//         // UDP 下一般不会走到这里，但保持与 TCP 对齐
//         if (err_no == EINPROGRESS)
//             return Result::Ok();

//         return Result::Err(
//             Error::transport()
//                 .code(err_no)
//                 .message("connect failed")
//                 .build());
//     }
// }