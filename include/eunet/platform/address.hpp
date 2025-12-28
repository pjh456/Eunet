#ifndef INCLUDE_EUNET_PLATFORM_ADDRESS
#define INCLUDE_EUNET_PLATFORM_ADDRESS

#include <string>
#include <vector>
#include <cstdint>
#include <netinet/in.h>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"

namespace platform::net
{
    enum class AddressFamily
    {
        IPv4,
        IPv6,
        Any
    };

    class SocketAddress;

    class SocketAddress
    {
    private:
        sockaddr_storage m_addr{};
        socklen_t m_len{0};

    public:
        static util::ResultV<std::vector<SocketAddress>>
        resolve(
            const std::string &host,
            uint16_t port,
            AddressFamily family = AddressFamily::Any);

        static SocketAddress
        from_ipv4(uint32_t addr_be, uint16_t port);

        static SocketAddress
        from_ipv6(const in6_addr &addr, uint16_t port);

        static SocketAddress
        any_ipv4(uint16_t port);

        static SocketAddress
        loopback_ipv4(uint16_t port);

    public:
        SocketAddress() = default;

        explicit SocketAddress(
            const sockaddr *addr,
            socklen_t len);

    public:
        const sockaddr *as_sockaddr() const noexcept;
        socklen_t length() const noexcept;

        int family() const noexcept;
    };
}

std::string to_string(
    const platform::net::SocketAddress &addr);
#endif