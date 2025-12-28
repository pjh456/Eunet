#include <cassert>
#include <arpa/inet.h>
#include <iostream>

#include "eunet/platform/address.hpp"

using platform::net::SocketAddress;

void test_from_ipv4()
{
    auto addr = SocketAddress::from_ipv4(
        htonl(0x7F000001), // 127.0.0.1
        8080);

    assert(addr.family() == AF_INET);
    assert(addr.length() == sizeof(sockaddr_in));

    auto *sa =
        reinterpret_cast<const sockaddr_in *>(
            addr.as_sockaddr());

    assert(sa->sin_family == AF_INET);
    assert(ntohs(sa->sin_port) == 8080);
    assert(sa->sin_addr.s_addr == htonl(0x7F000001));
}

void test_any_ipv4()
{
    auto addr = SocketAddress::any_ipv4(1234);

    auto *sa =
        reinterpret_cast<const sockaddr_in *>(
            addr.as_sockaddr());

    assert(sa->sin_addr.s_addr == INADDR_ANY);
    assert(ntohs(sa->sin_port) == 1234);
}

void test_loopback_ipv4()
{
    auto addr = SocketAddress::loopback_ipv4(80);

    auto *sa =
        reinterpret_cast<const sockaddr_in *>(
            addr.as_sockaddr());

    assert(sa->sin_addr.s_addr == htonl(INADDR_LOOPBACK));
    assert(ntohs(sa->sin_port) == 80);
}

void test_from_ipv6()
{
    in6_addr addr6{};
    assert(inet_pton(AF_INET6, "::1", &addr6) == 1);

    auto addr = SocketAddress::from_ipv6(addr6, 443);

    assert(addr.family() == AF_INET6);
    assert(addr.length() == sizeof(sockaddr_in6));

    auto *sa =
        reinterpret_cast<const sockaddr_in6 *>(
            addr.as_sockaddr());

    assert(sa->sin6_family == AF_INET6);
    assert(ntohs(sa->sin6_port) == 443);
}

void test_to_string_ipv4()
{
    auto addr =
        SocketAddress::from_ipv4(
            htonl(0x7F000001), 8080);

    assert(
        to_string(addr) ==
        std::string("127.0.0.1:8080"));
}

void test_to_string_ipv6()
{
    in6_addr addr6{};
    assert(inet_pton(AF_INET6, "::1", &addr6) == 1);

    auto addr =
        SocketAddress::from_ipv6(addr6, 80);

    assert(
        to_string(addr) ==
        std::string("[::1]:80"));
}

void test_resolve_localhost_any()
{
    auto res =
        SocketAddress::resolve(
            "localhost",
            12345);

    assert(res.is_ok());

    const auto &list = res.unwrap();
    assert(!list.empty());

    for (auto &addr : list)
    {
        assert(
            addr.family() == AF_INET ||
            addr.family() == AF_INET6);

        if (addr.family() == AF_INET)
        {
            auto *sa =
                reinterpret_cast<const sockaddr_in *>(
                    addr.as_sockaddr());
            assert(ntohs(sa->sin_port) == 12345);
        }
    }
}

void test_resolve_ipv4_only()
{
    auto res =
        SocketAddress::resolve(
            "localhost",
            80,
            platform::net::AddressFamily::IPv4);

    assert(res.is_ok());

    for (auto &addr : res.unwrap())
        assert(addr.family() == AF_INET);
}

void test_resolve_invalid_host()
{
    auto res =
        SocketAddress::resolve(
            "this-host-should-not-exist.invalid",
            1);

    assert(res.is_err());
}

int main()
{
    test_from_ipv4();
    test_any_ipv4();
    test_loopback_ipv4();
    test_from_ipv6();
    test_to_string_ipv4();
    test_to_string_ipv6();
    test_resolve_localhost_any();
    test_resolve_ipv4_only();
    test_resolve_invalid_host();

    std::cout << "[SocketAddress] all tests passed.\n";
    return 0;
}
