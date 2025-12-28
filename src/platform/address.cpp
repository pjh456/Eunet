#include "eunet/platform/address.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>

namespace platform::net
{
    util::ResultV<std::vector<SocketAddress>>
    SocketAddress::resolve(const std::string &host, uint16_t port)
    {
        using Result = util::ResultV<std::vector<SocketAddress>>;

        addrinfo hints{};
        hints.ai_family = AF_INET;       // IPv4
        hints.ai_socktype = SOCK_STREAM; // TCP
        hints.ai_flags = AI_ADDRCONFIG;

        addrinfo *res = nullptr;
        int err = getaddrinfo(host.c_str(), nullptr, &hints, &res);
        if (err != 0)
            return Result::Err(
                util::Error::from_errno(
                    err, "Failed to resolve host"));

        std::vector<SocketAddress> addrs;
        for (addrinfo *p = res; p != nullptr; p = p->ai_next)
        {
            if (p->ai_family == AF_INET)
            {
                sockaddr_in sa{};
                sa.sin_family = AF_INET;
                sa.sin_port = htons(port);
                sa.sin_addr = reinterpret_cast<sockaddr_in *>(p->ai_addr)->sin_addr;
                addrs.emplace_back(sa);
            }
        }

        freeaddrinfo(res);

        if (addrs.empty())
            return Result::Err(
                util::Error::internal(
                    "No IPv4 address found for host " + host));

        return Result::Ok(std::move(addrs));
    }

    SocketAddress::SocketAddress(const sockaddr_in &addr)
        : addr(addr) {}

    const sockaddr *
    SocketAddress::as_sockaddr() const { return reinterpret_cast<const sockaddr *>(&addr); }

    socklen_t
    SocketAddress::length() const { return sizeof(addr); }
}

std::string to_string(
    const platform::net::SocketAddress &addr)
{
    char buf[INET_ADDRSTRLEN];
    const sockaddr_in *sa = reinterpret_cast<const sockaddr_in *>(addr.as_sockaddr());
    if (inet_ntop(
            AF_INET,
            &sa->sin_addr,
            buf, sizeof(buf)) == nullptr)
        return "<invalid address>";

    std::ostringstream oss;
    oss << buf << ":" << ntohs(sa->sin_port);
    return oss.str();
}