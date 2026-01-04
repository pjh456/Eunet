/*
 * ============================================================================
 *  File Name   : endpoint.hpp
 *  Module      : platform/net
 *
 *  Description :
 *      IP 地址与端口的抽象 (sockaddr_storage 封装)。
 *      支持 IPv4 与 IPv6 的统一表达，提供从字符串解析和转换为字符串的功能。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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