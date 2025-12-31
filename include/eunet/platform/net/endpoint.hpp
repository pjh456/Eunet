#ifndef INCLUDE_EUNET_PLATFORM_NET_ENDPOINT
#define INCLUDE_EUNET_PLATFORM_NET_ENDPOINT

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdint>
#include <string>
#include <string_view>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"

namespace platform::net
{
    class Endpoint;
    using EndpointResult = util::ResultV<Endpoint>;

    class Endpoint
    {
    private:
        sockaddr_storage m_addr{};
        socklen_t m_len{0};

    public:
        static EndpointResult
        from_string(
            std::string_view ip,
            uint16_t port);

        static Endpoint from_ipv4(const uint32_t &addr_be, uint16_t port);
        static Endpoint from_ipv6(const in6_addr &addr, uint16_t port);

        static Endpoint any_ipv4(uint16_t port);
        static Endpoint loopback_ipv4(uint16_t port);

    public:
        Endpoint() = default;
        Endpoint(const sockaddr *sa, socklen_t len);

    public:
        const sockaddr *as_sockaddr() const noexcept;
        uint16_t port() const noexcept;
        socklen_t length() const noexcept;
        int family() const noexcept;

    public:
        bool operator==(const Endpoint &other) const noexcept;
    };
}

std::string to_string(const platform::net::Endpoint &ep);

#endif // INCLUDE_EUNET_PLATFORM_NET_ENDPOINT