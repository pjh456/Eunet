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
            return util::ResultV<void>::Err(
                util::Error::protocol()
                    .message("HTTP GET failed")
                    .context("HttpGetScenario")
                    .wrap(res.unwrap_err())
                    .build());
        }

        return util::ResultV<void>::Ok();
    }
}