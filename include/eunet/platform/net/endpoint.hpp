#ifndef INCLUDE_EUNET_PLATFORM_NET_ENDPOINT
#define INCLUDE_EUNET_PLATFORM_NET_ENDPOINT

#include <sys/socket.h>
#include <cstdint>
#include <string>
#include <string_view>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"

namespace platform::net
{
    class Endpoint
    {
    private:
        sockaddr_storage m_addr{};
        socklen_t m_len{0};

    public:
        static util::ResultV<Endpoint>
        from_string(
            std::string_view ip,
            uint16_t port);

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