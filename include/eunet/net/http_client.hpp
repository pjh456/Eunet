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