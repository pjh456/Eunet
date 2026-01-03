#ifndef INCLUDE_EUNET_NET_HTTP_SCENARIO
#define INCLUDE_EUNET_NET_HTTP_SCENARIO

#include <string>
#include <cstdint>
#include <vector>

#include "fmt/format.h"

#include "eunet/core/scenario.hpp"

namespace net::http
{
    struct HttpConfig
    {
        // ---------------- user input ----------------
        std::string method = "GET";
        std::string url;

        // ---------------- parsed result ----------------
        std::string scheme = "http"; // http / https
        std::string host;
        uint16_t port = 0;      // 0 = not decided yet
        std::string path = "/"; // path + query
    };

    class HttpGetScenario : public core::scenario::Scenario
    {
    private:
        HttpConfig config_;

        void parse_url();

    public:
        explicit HttpGetScenario(std::string url)
        {
            config_.url = std::move(url);
            parse_url();
        }

        util::ResultV<void> run(
            core::Orchestrator &orch) override;
    };
}

#endif // INCLUDE_EUNET_NET_HTTP_SCENARIO