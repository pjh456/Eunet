/*
 * ============================================================================
 *  File Name   : http_client.hpp
 *  Module      : net/http
 *
 *  Description :
 *      HTTP 客户端实现。利用 Boost.Beast 进行 HTTP 协议的序列化与解析，
 *      利用底层的 TCPClient 进行数据传输，并负责向 Orchestrator 汇报
 *      HTTP 层的细粒度事件（如 Headers Received）。
 *
 *  Third-Party Dependencies :
 *      - Boost.Beast
 *          Usage     : HTTP 协议解析与构建
 *          License   : Boost Software License 1.0
 *
 *      - Boost.Asio
 *          Usage     : Beast 底层依赖的 Buffer 接口
 *          License   : Boost Software License 1.0
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_NET_HTTP_CLIENT
#define INCLUDE_EUNET_NET_HTTP_CLIENT

#include "eunet/core/orchestrator.hpp"
#include "eunet/util/result.hpp"
#include "eunet/net/tcp_client.hpp"
#include "eunet/net/http/http_request.hpp"
#include "eunet/net/http/http_response.hpp"

namespace net::http
{
    class HTTPClient
    {
    public:
        explicit HTTPClient(core::Orchestrator &orch);

        util::ResultV<HttpResponse> get(const HttpRequest &req);

    private:
        core::Orchestrator &orch;
        net::tcp::TCPClient tcp;

        util::ResultV<void> emit(const core::Event &e);
    };
}

#endif // INCLUDE_EUNET_NET_HTTP_CLIENT