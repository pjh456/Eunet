/*
 * ============================================================================
 *  File Name   : http_scenario.hpp
 *  Module      : net/http
 *
 *  Description :
 *      HTTP GET 请求场景的具体实现。
 *      负责解析用户输入的 URL，配置 HTTP Client，并执行完整的请求流程。
 *      是 Scenario 接口的具体实现之一。
 *
 *  Third-Party Dependencies :
 *      - fmt
 *          Usage     : 格式化日志或错误信息
 *          License   : MIT License
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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