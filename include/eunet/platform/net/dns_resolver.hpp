/*
 * ============================================================================
 *  File Name   : dns_resolver.hpp
 *  Module      : platform/net
 *
 *  Description :
 *      DNS 解析器封装。主要包装了 getaddrinfo 系统调用，
 *      将域名转换为 Endpoint 列表。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_PLATFORM_NET_DNS_RESOLVER
#define INCLUDE_EUNET_PLATFORM_NET_DNS_RESOLVER

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/platform/net/common.hpp"
#include "eunet/platform/net/endpoint.hpp"

namespace platform::net
{
    class DNSResolver
    {
    public:
        using EndpointList = std::vector<Endpoint>;
        using ResolveResult = util::ResultV<EndpointList>;

    public:
        // 职责：只负责把域名变 Endpoint
        static ResolveResult
        resolve(
            std::string_view host,
            uint16_t port,
            AddressFamily family = AddressFamily::Any);
    };
}

#endif // INCLUDE_EUNET_PLATFORM_NET_DNS_RESOLVER