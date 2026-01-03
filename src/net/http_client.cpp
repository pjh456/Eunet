#include "eunet/net/http_client.hpp"

#include <sstream>

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

namespace net::http
{
    namespace beast = boost::beast;
    namespace http = beast::http;

    HTTPClient::HTTPClient(core::Orchestrator &o)
        : orch(o), tcp(o) {}

    util::ResultV<void>
    HTTPClient::emit(const core::Event &e)
    {
        return orch.emit(e);
    }

    util::ResultV<HttpResponse>
    HTTPClient::get(const HttpRequest &cfg)
    {
        // ---------------- connect ----------------
        {
            auto r = tcp.connect(cfg.host, cfg.port);
            if (r.is_err())
                return util::ResultV<HttpResponse>::Err(r.unwrap_err());
        }

        (void)emit(core::Event::info(
            core::EventType::HTTP_REQUEST_BUILD,
            "HTTP GET " + cfg.target));

        // ---------------- build request ----------------
        http::request<http::string_body> req{
            http::verb::get,
            cfg.target,
            11};

        req.set(http::field::host, cfg.host);
        req.set(http::field::user_agent, "EuNet/0.1");
        req.set(http::field::connection, "close");

        for (auto &[k, v] : cfg.headers)
            req.set(k, v);

        req.prepare_payload();

        // ---------------- serialize & send ----------------
        std::string req_text;
        {
            std::ostringstream oss;
            oss << req;
            req_text = oss.str();
        }

        std::vector<std::byte> send_buf(
            reinterpret_cast<const std::byte *>(req_text.data()),
            reinterpret_cast<const std::byte *>(req_text.data() + req_text.size()));

        {
            auto r = tcp.send(send_buf);
            if (r.is_err())
            {
                tcp.close();
                return util::ResultV<HttpResponse>::Err(r.unwrap_err());
            }
        }

        emit(core::Event::info(
            core::EventType::HTTP_SENT,
            "HTTP request sent"));

        // ---------------- receive & parse ----------------
        beast::flat_buffer read_buf;
        http::response_parser<http::dynamic_body> parser;
        parser.body_limit(16 * 1024 * 1024);

        std::vector<std::byte> buf(4096);
        while (!parser.is_done())
        {
            auto r = tcp.recv(buf, buf.size());

            if (r.is_ok())
            {
                auto n = r.unwrap();
                auto bytes = boost::asio::buffer(buf.data(), n);

                boost::system::error_code ec;
                parser.put(bytes, ec);

                if (ec)
                {
                    tcp.close();
                    return util::ResultV<HttpResponse>::Err(
                        util::Error::protocol()
                            .message("HTTP parse error")
                            .context(ec.message())
                            .build());
                }

                if (parser.is_done())
                    break;

                continue;
            }

            // ---------- recv returned Error ----------
            const auto &err = r.unwrap_err();

            if (err.category() == util::ErrorCategory::PeerClosed)
            {
                if (parser.is_done())
                    break;

                tcp.close();
                return util::ResultV<HttpResponse>::Err(
                    util::Error::protocol()
                        .message("Connection closed before HTTP response completed")
                        .context("TCP EOF")
                        .build());
            }

            // ---------- real failure ----------
            tcp.close();
            return util::ResultV<HttpResponse>::Err(err);
        }

        tcp.close();

        // ---------------- build response ----------------
        auto res = parser.get();

        HttpResponse out;
        out.status = res.result_int();

        for (auto const &h : res.base())
            out.headers.emplace(h.name_string(), h.value());

        out.body = beast::buffers_to_string(res.body().data());

        (void)emit(core::Event::info(
            core::EventType::HTTP_BODY_DONE,
            "HTTP body received (" + std::to_string(out.body.size()) + " bytes)"));

        return util::ResultV<HttpResponse>::Ok(std::move(out));
    }
}
