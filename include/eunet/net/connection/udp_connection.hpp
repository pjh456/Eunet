// #ifndef INCLUDE_EUNET_NET_CONNECTION_UDP_CONNECTION
// #define INCLUDE_EUNET_NET_CONNECTION_UDP_CONNECTION

// #include "eunet/platform/time.hpp"
// #include "eunet/platform/socket/udp_socket.hpp"
// #include "eunet/net/io_buffer.hpp"
// #include "eunet/net/connection.hpp"

// namespace net::udp
// {
//     class UDPConnection final : public Connection
//     {
//     private:
//         platform::net::UDPSocket sock;

//     public:
//         // 创建并 connect 到 peer
//         static util::ResultV<UDPConnection>
//         connect(const platform::net::SocketAddress &peer);

//         // 从已有 socket 构造（server / 复用场景）
//         static UDPConnection
//         from_socket(platform::net::UDPSocket &&sock);

//     private:
//         explicit UDPConnection(platform::net::UDPSocket &&sock);

//     public:
//         UDPConnection(UDPConnection &&) noexcept = default;
//         UDPConnection &operator=(UDPConnection &&) noexcept = default;
//         ~UDPConnection() override = default;

//     public:
//         platform::fd::FdView fd() const noexcept override;
//         void close() noexcept override;
//         bool is_open() const noexcept override;

//     public:
//         util::ResultV<size_t>
//         read(std::byte *buf, size_t len,
//              platform::time::Duration timeout) override;

//         util::ResultV<size_t>
//         write(const std::byte *buf, size_t len,
//               platform::time::Duration timeout) override;

//         bool has_pending_output() const noexcept override { return false; }
//     };
// }

// #endif // INCLUDE_EUNET_NET_CONNECTION_UDP_CONNECTION