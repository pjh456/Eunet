#include "eunet/platform/address.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <sstream>

namespace platform::net
{
    util::ResultV<std::vector<SocketAddress>>
    SocketAddress::resolve(
        const std::string &host,
        uint16_t port,
        AddressFamily family)
    {
        using Result = util::ResultV<std::vector<SocketAddress>>;

        addrinfo hints{};
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_ADDRCONFIG;

        switch (family)
        {
        case AddressFamily::IPv4:
            hints.ai_family = AF_INET;
            break;
        case AddressFamily::IPv6:
            hints.ai_family = AF_INET6;
            break;
        case AddressFamily::Any:
            hints.ai_family = AF_UNSPEC;
            break;
        }

        addrinfo *res = nullptr;
        int err = ::getaddrinfo(
            host.c_str(),
            nullptr,
            &hints,
            &res);
        if (err != 0)
        {
            return Result::Err(
                util::Error::create()
                    .system()
                    .code(err)
                    .message("getaddrinfo failed")
                    .build());
        }

        std::vector<SocketAddress> out;

        for (addrinfo *p = res; p; p = p->ai_next)
        {
            if (p->ai_family == AF_INET)
            {
                sockaddr_in sa =
                    *reinterpret_cast<sockaddr_in *>(p->ai_addr);
                sa.sin_port = htons(port);
                out.emplace_back(
                    reinterpret_cast<sockaddr *>(&sa),
                    sizeof(sa));
            }
            else if (p->ai_family == AF_INET6)
            {
                sockaddr_in6 sa =
                    *reinterpret_cast<sockaddr_in6 *>(p->ai_addr);
                sa.sin6_port = htons(port);
                out.emplace_back(
                    reinterpret_cast<sockaddr *>(&sa),
                    static_cast<socklen_t>(sizeof(sa)));
            }
        }

        freeaddrinfo(res);

        if (out.empty())
        {
            return Result::Err(
                util::Error::create()
                    .dns()
                    .message("No address resolved for " + host)
                    .build());
        }

        return Result::Ok(std::move(out));
    }

    SocketAddress
    SocketAddress::from_ipv4(
        uint32_t addr_be,
        uint16_t port)
    {
        sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = addr_be;
        sa.sin_port = htons(port);
        return SocketAddress(
            reinterpret_cast<sockaddr *>(&sa),
            static_cast<socklen_t>(sizeof(sa)));
    }

    SocketAddress
    SocketAddress::from_ipv6(
        const in6_addr &addr,
        uint16_t port)
    {
        sockaddr_in6 sa{};
        sa.sin6_family = AF_INET6;
        sa.sin6_addr = addr;
        sa.sin6_port = htons(port);
        return SocketAddress(
            reinterpret_cast<sockaddr *>(&sa),
            sizeof(sa));
    }

    SocketAddress
    SocketAddress::any_ipv4(uint16_t port)
    {
        return from_ipv4(INADDR_ANY, port);
    }

    SocketAddress
    SocketAddress::loopback_ipv4(uint16_t port)
    {
        return from_ipv4(htonl(INADDR_LOOPBACK), port);
    }

    SocketAddress::SocketAddress(
        const sockaddr *addr,
        socklen_t len)
        : m_len(len) { std::memcpy(&m_addr, addr, len); }

    const sockaddr *
    SocketAddress::as_sockaddr()
        const noexcept { return reinterpret_cast<const sockaddr *>(&m_addr); }

    socklen_t
    SocketAddress::length()
        const noexcept { return m_len; }

    int
    SocketAddress::family()
        const noexcept { return m_addr.ss_family; }

}

std::string to_string(
    const platform::net::SocketAddress &addr)
{
    char buf[INET6_ADDRSTRLEN]{};

    std::ostringstream oss;
    if (addr.family() == AF_INET)
    {
        auto *sa =
            reinterpret_cast<
                const sockaddr_in *>(addr.as_sockaddr());
        inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
        oss << buf << ":" << ntohs(sa->sin_port);
    }
    else if (addr.family() == AF_INET6)
    {
        auto *sa =
            reinterpret_cast<
                const sockaddr_in6 *>(addr.as_sockaddr());
        inet_ntop(AF_INET6, &sa->sin6_addr, buf, sizeof(buf));
        oss << "[" << buf << "]:" << ntohs(sa->sin6_port);
    }
    else
        oss << "<unknown address>";

    return oss.str();
}