#include "eunet/platform/net/endpoint.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <sstream>

namespace platform::net
{
    util::ResultV<Endpoint>
    Endpoint::from_string(
        std::string_view ip,
        uint16_t port)
    {
        sockaddr_in sa4{};
        if (::inet_pton(AF_INET, ip.data(), &sa4.sin_addr) == 1)
        {
            sa4.sin_family = AF_INET;
            sa4.sin_port = htons(port);
            return util::ResultV<Endpoint>::Ok(
                Endpoint(
                    reinterpret_cast<sockaddr *>(&sa4),
                    sizeof(sa4)));
        }

        sockaddr_in6 sa6{};
        if (::inet_pton(AF_INET6, ip.data(), &sa6.sin6_addr) == 1)
        {
            sa6.sin6_family = AF_INET6;
            sa6.sin6_port = htons(port);
            return util::ResultV<Endpoint>::Ok(
                Endpoint(
                    reinterpret_cast<sockaddr *>(&sa6),
                    sizeof(sa6)));
        }

        return util::ResultV<Endpoint>::Err(
            util::Error::system()
                .message("invalid IP address")
                .build());
    }

    Endpoint::Endpoint(
        const sockaddr *sa,
        socklen_t len) : m_len(len)
    {
        std::memcpy(&m_addr, sa, len);
    }

    const sockaddr *
    Endpoint::as_sockaddr() const noexcept
    {
        return reinterpret_cast<
            const sockaddr *>(&m_addr);
    }

    uint16_t Endpoint::port() const noexcept
    {
        if (m_addr.ss_family == AF_INET)
        {
            auto *sa = reinterpret_cast<const sockaddr_in *>(&m_addr);
            return ntohs(sa->sin_port);
        }
        if (m_addr.ss_family == AF_INET6)
        {
            auto *sa = reinterpret_cast<const sockaddr_in6 *>(&m_addr);
            return ntohs(sa->sin6_port);
        }
        return 0;
    }

    socklen_t Endpoint::length() const noexcept { return m_len; }

    int Endpoint::family() const noexcept { return m_addr.ss_family; }

    bool
    Endpoint::operator==(const Endpoint &other)
        const noexcept
    {
        return m_len == other.m_len &&
               std::memcmp(&m_addr, &other.m_addr, m_len) == 0;
    }
}

std::string
to_string(const platform::net::Endpoint &ep)
{
    char buf[INET6_ADDRSTRLEN]{};
    std::ostringstream oss;

    if (ep.family() == AF_INET)
    {
        auto *sa =
            reinterpret_cast<const sockaddr_in *>(ep.as_sockaddr());
        ::inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
        oss << buf << ":" << ntohs(sa->sin_port);
    }
    else if (ep.family() == AF_INET6)
    {
        auto *sa =
            reinterpret_cast<const sockaddr_in6 *>(ep.as_sockaddr());
        ::inet_ntop(AF_INET6, &sa->sin6_addr, buf, sizeof(buf));
        oss << "[" << buf << "]:" << ntohs(sa->sin6_port);
    }
    else
    {
        oss << "<unknown endpoint>";
    }

    return oss.str();
}
