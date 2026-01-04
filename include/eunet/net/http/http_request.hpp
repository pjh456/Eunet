/*
 * ============================================================================
 *  File Name   : http_request.hpp
 *  Module      : net/http
 *
 *  Description :
 *      HTTP 请求的数据结构定义。包含 Host、Port、Target (Path)
 *      以及 Headers Map，用于参数传递。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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
        int timeout_ms = 3000;
        bool connection_close = true;
    };
}

#endif // INCLUDE_EUNET_NET_HTTP_REQUEST