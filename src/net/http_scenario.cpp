/*
 * ============================================================================
 *  File Name   : http_scenario.cpp
 *  Module      : net/http
 *
 *  Description :
 *      HTTP 场景逻辑。包含简单的 URL 解析器（Scheme/Host/Port/Path），
 *      驱动 HTTPClient 执行 GET 请求并处理可能的错误。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#include "eunet/net/http_scenario.hpp"
#include "eunet/net/http_client.hpp"

namespace net::http
{
    void HttpGetScenario::parse_url()
    {
        std::string_view url = config_.url;

        // ---------------- scheme ----------------
        config_.scheme = "http";
        if (auto pos = url.find("://"); pos != std::string_view::npos)
        {
            config_.scheme = std::string(url.substr(0, pos));
            url.remove_prefix(pos + 3);
        }

        // ---------------- authority ----------------
        std::string_view authority;
        if (auto slash = url.find('/'); slash != std::string_view::npos)
        {
            authority = url.substr(0, slash);
            url.remove_prefix(slash);
        }
        else
        {
            authority = url;
            url = "/";
        }

        // ---------------- host / port ----------------
        if (auto colon = authority.find(':'); colon != std::string_view::npos)
        {
            config_.host = std::string(authority.substr(0, colon));
            config_.port = static_cast<uint16_t>(
                std::stoi(std::string(authority.substr(colon + 1))));
        }
        else
        {
            config_.host = std::string(authority);
            config_.port = (config_.scheme == "https") ? 443 : 80;
        }

        // ---------------- path + query ----------------
        if (auto hash = url.find('#'); hash != std::string_view::npos)
            url = url.substr(0, hash);

        config_.path = url.empty() ? "/" : std::string(url);
    }

    util::ResultV<void>
    HttpGetScenario::run(
        core::Orchestrator &orch)
    {
        HTTPClient client(orch);

        auto res = client.get(
            {.host = config_.host,
             .port = config_.port,
             .target = config_.path});

        if (res.is_err())
        {
            auto err = res.unwrap_err();
            if (err.category() != util::ErrorCategory::PeerClosed)
            {
                (void)orch.emit(
                    core::Event::failure(
                        core::EventType::CONNECTION_IDLE,
                        err));

                return util::ResultV<void>::Err(
                    util::Error::protocol()
                        .message("HTTP GET failed")
                        .context("HttpGetScenario")
                        .wrap(res.unwrap_err())
                        .build());
            }
        }

        return util::ResultV<void>::Ok();
    }
}