/*
 * ============================================================================
 *  File Name   : dns_resolver.cpp
 *  Module      : platform/net
 *
 *  Description :
 *      DNS Resolver 实现。调用 getaddrinfo 并将结果链表转换为 C++ 友好的
 *      Endpoint 向量，统一处理 IPv4/IPv6 结果。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#include "eunet/platform/net/dns_resolver.hpp"

#include <netdb.h>

namespace platform::net
{
    DNSResolver::ResolveResult
    DNSResolver::resolve(
        std::string_view host,
        uint16_t port,
        AddressFamily family)
    {
        using Ret = ResolveResult;
        using util::Error;

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
        default:
            hints.ai_family = AF_UNSPEC;
            break;
        }

        addrinfo *res = nullptr;
        int err = ::getaddrinfo(
            std::string(host).c_str(),
            nullptr,
            &hints,
            &res);

        if (err != 0)
        {
            return Ret::Err(
                Error::dns()
                    .code(err)
                    .set_category(from_gai_error(err))
                    .message(gai_strerror(err))
                    .context(std::string(host))
                    .build());
        }

        EndpointList out;

        for (addrinfo *p = res; p; p = p->ai_next)
        {
            if (p->ai_family == AF_INET)
            {
                auto sa = *reinterpret_cast<sockaddr_in *>(p->ai_addr);
                out.push_back(Endpoint::from_ipv4(sa.sin_addr.s_addr, port));
            }
            else if (p->ai_family == AF_INET6)
            {
                auto sa = *reinterpret_cast<sockaddr_in6 *>(p->ai_addr);
                out.push_back(Endpoint::from_ipv6(sa.sin6_addr, port));
            }
        }

        ::freeaddrinfo(res);

        if (out.empty())
        {
            return Ret::Err(
                Error::dns()
                    .target_not_found()
                    .message("DNS query returned no addresses")
                    .context(std::string(host))
                    .build());
        }

        return Ret::Ok(std::move(out));
    }
}