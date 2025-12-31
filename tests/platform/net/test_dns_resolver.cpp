#include "eunet/platform/net/endpoint.hpp"
#include "eunet/platform/net/dns_resolver.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <cassert>
#include <cstring>

using namespace platform::net;

void test_endpoint_ipv4()
{
    uint32_t addr = htonl(0x7f000001); // 127.0.0.1
    Endpoint ep = Endpoint::from_ipv4(addr, 8080);

    assert(ep.family() == AF_INET);
    assert(ep.port() == 8080);

    auto *sa = reinterpret_cast<const sockaddr_in *>(ep.as_sockaddr());
    assert(sa->sin_addr.s_addr == addr);

    std::cout << "IPv4 Endpoint: " << to_string(ep) << "\n";
}

void test_endpoint_ipv6()
{
    in6_addr addr{};
    inet_pton(AF_INET6, "::1", &addr);
    Endpoint ep = Endpoint::from_ipv6(addr, 9090);

    assert(ep.family() == AF_INET6);
    assert(ep.port() == 9090);

    char buf[INET6_ADDRSTRLEN]{};
    inet_ntop(AF_INET6, &addr, buf, sizeof(buf));
    std::cout << "IPv6 Endpoint: " << to_string(ep) << "\n";
}

void test_endpoint_from_string()
{
    auto r1 = Endpoint::from_string("127.0.0.1", 8000);
    assert(r1.is_ok());
    Endpoint ep4 = r1.unwrap();
    assert(ep4.family() == AF_INET);
    assert(ep4.port() == 8000);

    auto r2 = Endpoint::from_string("::1", 8001);
    assert(r2.is_ok());
    Endpoint ep6 = r2.unwrap();
    assert(ep6.family() == AF_INET6);
    assert(ep6.port() == 8001);

    auto r3 = Endpoint::from_string("invalid_ip", 1234);
    assert(r3.is_err());

    std::cout << "Endpoint from_string tests passed.\n";
}

void test_dns_resolver()
{
    auto res = DNSResolver::resolve("localhost", 80);
    assert(res.is_ok());
    auto eps = res.unwrap();

    bool found_ipv4 = false;
    bool found_ipv6 = false;

    for (auto &ep : eps)
    {
        std::cout << "Resolved: " << to_string(ep) << "\n";
        if (ep.family() == AF_INET)
            found_ipv4 = true;
        if (ep.family() == AF_INET6)
            found_ipv6 = true;
    }

    assert(found_ipv4 || found_ipv6);

    auto res_invalid = DNSResolver::resolve("this-domain-should-not-exist-1234.local", 80);
    assert(res_invalid.is_err());

    std::cout << "DNS Resolver tests passed.\n";
}

void test_endpoint_comparison()
{
    auto ep1 = Endpoint::loopback_ipv4(1234);
    auto ep2 = Endpoint::from_string("127.0.0.1", 1234).unwrap();

    assert(ep1 == ep2);

    auto ep3 = Endpoint::any_ipv4(1234);
    assert(!(ep1 == ep3));

    std::cout << "Endpoint comparison tests passed.\n";
}

int main()
{
    std::cout << "Starting Endpoint and DNSResolver tests...\n";

    test_endpoint_ipv4();
    test_endpoint_ipv6();
    test_endpoint_from_string();
    test_dns_resolver();
    test_endpoint_comparison();

    std::cout << "All tests passed successfully.\n";
    return 0;
}
