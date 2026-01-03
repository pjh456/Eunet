#ifndef INCLUDE_EUNET_NET_HTTP_CLIENT
#define INCLUDE_EUNET_NET_HTTP_CLIENT

#include <string>
#include <map>

#include "eunet/core/orchestrator.hpp"
#include "eunet/util/result.hpp"
#include "eunet/net/tcp_client.hpp"

namespace net::http
{
    struct HttpRequest
    {
        std::string host;
        uint16_t port = 80;
        std::string target = "/";
        std::map<std::string, std::string> headers;
    };

    struct HttpResponse
    {
        // ---------------- status ----------------
        int status = 0;     // e.g. 200
        std::string reason; // e.g. "OK"

        // ---------------- headers ----------------
        // 说明：
        // - key 使用小写，方便查找
        // - value 保留原始值
        std::map<std::string, std::string> headers;

        // ---------------- body ----------------
        std::string body;

        // ---------------- helpers ----------------
        bool ok() const noexcept { return status >= 200 && status < 300; }

        size_t body_size() const noexcept { return body.size(); }

        std::string header(const std::string &key) const
        {
            auto it = headers.find(key);
            return it == headers.end() ? std::string{} : it->second;
        }
    };

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