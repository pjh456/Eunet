#ifndef INCLUDE_EUNET_NET_HTTP_SCENARIO
#define INCLUDE_EUNET_NET_HTTP_SCENARIO

#include <string>
#include <cstdint>
#include <vector>

#include "fmt/format.h"

#include "eunet/core/scenario.hpp"
#include "eunet/net/async_tcp_client.hpp"

namespace net::http
{
    struct HttpConfig
    {
        std::string method = "GET";
        std::string url;
        std::string host;
        uint16_t port = 80;
        std::string path = "/";
    };

    class HttpGetScenario : public core::scenario::Scenario
    {
    private:
        HttpConfig config_;

        // 简单的解析工具（实际可引入更完善的 URL Parser）
        void parse_url()
        {
            // 示例逻辑：简单拆分 host 和 path
            size_t pos = config_.url.find("//");
            std::string remaining = (pos == std::string::npos) ? config_.url : config_.url.substr(pos + 2);
            size_t slash_pos = remaining.find('/');
            if (slash_pos != std::string::npos)
            {
                config_.host = remaining.substr(0, slash_pos);
                config_.path = remaining.substr(slash_pos);
            }
            else
            {
                config_.host = remaining;
                config_.path = "/";
            }
        }

    public:
        explicit HttpGetScenario(std::string url)
        {
            config_.url = std::move(url);
            parse_url();
        }

        util::ResultV<void> run(core::Orchestrator &orch) override
        {
            tcp::AsyncTCPClient client(orch);

            // 1. 连接
            auto res = client.connect(config_.host, config_.port);
            if (res.is_err())
                return util::ResultV<void>::Ok();

            // 2. 构造并发送 Request
            std::string req = fmt::format(
                "{} {} HTTP/1.1\r\n"
                "Host: {}\r\n"
                "Connection: close\r\n"
                "User-Agent: EuNet/1.0\r\n\r\n",
                config_.method, config_.path, config_.host);

            std::vector<std::byte> data;
            for (char c : req)
                data.push_back(static_cast<std::byte>(c));
            (void)client.send(data);

            // 3. 接收 Response (流式接收直到关闭)
            std::vector<std::byte> buf;
            while (true)
            {
                auto r_res = client.recv(buf, 4096);
                if (r_res.is_err() || r_res.unwrap() == 0)
                    break;
            }

            client.close();
            return util::ResultV<void>::Ok();
        }
    };
}

#endif // INCLUDE_EUNET_NET_HTTP_SCENARIO