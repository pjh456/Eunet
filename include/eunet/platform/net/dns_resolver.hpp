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