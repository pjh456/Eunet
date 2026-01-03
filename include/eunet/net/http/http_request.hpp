#ifndef INCLUDE_EUNET_NET_HTTP_REQUEST
#define INCLUDE_EUNET_NET_HTTP_REQUEST

#include <cstdint>
#include <string>
#include <map>

namespace net::http
{
    struct HttpRequest
    {
        std::string host;
        uint16_t port = 80;
        std::string target = "/";
        std::map<std::string, std::string> headers;
    };
}

#endif // INCLUDE_EUNET_NET_HTTP_REQUEST