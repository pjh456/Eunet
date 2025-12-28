#ifndef INCLUDE_EUNET_PLATFORM_ADDRESS
#define INCLUDE_EUNET_PLATFORM_ADDRESS

#include <string>
#include <vector>
#include <netinet/in.h>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"

namespace platform::net
{
    class SocketAddress;

    class SocketAddress
    {
    private:
        sockaddr_in addr;

    public:
        static util::ResultV<std::vector<SocketAddress>>
        resolve(const std::string &host, uint16_t port);

    public:
        explicit SocketAddress(const sockaddr_in &addr);

    public:
        const sockaddr *as_sockaddr() const;
        socklen_t length() const;
    };
}

std::string to_string(
    const platform::net::SocketAddress &addr);
#endif