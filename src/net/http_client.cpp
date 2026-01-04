/*
 * ============================================================================
 *  File Name   : http_client.cpp
 *  Module      : net/http
 *
 *  Description :
 *      HTTP 客户端核心逻辑实现。使用 Boost.Beast 序列化请求，
 *      通过 TCPClient 发送，解析响应数据，并在关键节点（如 Headers Received）
 *      触发业务事件。
 *
 *  Third-Party Dependencies :
 *      - Boost.Beast
 *          Usage     : HTTP 协议处理
 *          License   : Boost Software License 1.0
 *
 *      - Boost.Asio
 *          Usage     : 缓冲区适配
 *          License   : Boost Software License 1.0
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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
        bool headers_emitted = false;

        // 首先建立 TCP 连接 此处复用 TCPClient 的逻辑
        {
            auto r = tcp.connect(cfg.host, cfg.port);
            if (r.is_err())
                return util::ResultV<HttpResponse>::Err(r.unwrap_err());
        }

        // 上报构建请求事件
        (void)emit(core::Event::info(
            core::EventType::HTTP_REQUEST_BUILD,
            "HTTP GET " + cfg.target));

        // 使用 Boost.Beast 构建 HTTP 请求对象 设置头部信息
        http::request<http::string_body> req{
            http::verb::get,
            cfg.target,
            11};

        req.set(http::field::host, cfg.host);
        req.set(http::field::user_agent, "EuNet/0.1");
        if (cfg.connection_close)
            req.set(http::field::connection, "close");

        for (auto &[k, v] : cfg.headers)
            req.set(k, v);

        req.prepare_payload();

        // 将请求序列化为文本 并转换为字节数组
        std::string req_text;
        {
            std::ostringstream oss;
            oss << req;
            req_text = oss.str();
        }

        std::vector<std::byte> send_buf(
            reinterpret_cast<const std::byte *>(req_text.data()),
            reinterpret_cast<const std::byte *>(req_text.data() + req_text.size()));

        // 通过 TCP 连接发送请求数据
        {
            auto r = tcp.send(send_buf);
            if (r.is_err())
            {
                auto err = r.unwrap_err();
                if (err.category() != util::ErrorCategory::PeerClosed)
                {
                    tcp.close();
                    return util::ResultV<HttpResponse>::Err(r.unwrap_err());
                }
            }
        }

        // 上报请求已发送事件
        (void)emit(core::Event::info(
            core::EventType::HTTP_SENT,
            "HTTP request sent"));

        // 初始化 HTTP 响应解析器
        beast::flat_buffer read_buf;
        http::response_parser<http::dynamic_body> parser;
        parser.body_limit(16 * 1024 * 1024);

        // 循环读取数据直到解析完成
        std::vector<std::byte> buf(4096);
        while (!parser.is_done())
        {
            // 从 TCP 接收数据到临时缓冲区
            auto r = tcp.recv(buf, buf.size());

            if (r.is_ok())
            {
                // 将接收到的数据喂给 Beast 解析器
                auto n = r.unwrap();
                if (n == 0)
                    break;

                auto bytes = boost::asio::buffer(buf.data(), n);

                boost::system::error_code ec;
                parser.put(bytes, ec);

                // 如果是 "需要更多数据"，则不是错误，继续循环读取即可
                if (ec == beast::http::error::need_more)
                {
                    ec = {}; // 清除错误状态
                }

                // 如果头部解析刚刚完成 上报头部接收事件
                if (parser.is_header_done() && !headers_emitted)
                {
                    (void)emit(
                        core::Event::info(
                            core::EventType::HTTP_HEADERS_RECEIVED,
                            "..."));
                    headers_emitted = true;
                }

                // 检查解析器错误
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

            // 处理接收错误 特别是 PeerClosed 对应 EOF 情况
            // 需要告知解析器数据流已结束 以便它完成最后的解析
            const auto &err = r.unwrap_err();

            if (err.category() == util::ErrorCategory::PeerClosed)
            {
                // 告知 parser：输入已结束（EOF）
                boost::system::error_code ec;
                parser.put(boost::asio::const_buffer{}, ec);

                // 如果是 "需要更多数据"，则不是错误，继续循环读取即可
                if (ec == beast::http::error::need_more)
                {
                    ec = {}; // 清除错误状态
                }

                if (ec)
                {
                    tcp.close();
                    return util::ResultV<HttpResponse>::Err(
                        util::Error::protocol()
                            .message("HTTP parse error on EOF")
                            .context(ec.message())
                            .build());
                }

                // 现在 parser 很可能已经 is_done()
                break;
            }

            // ---------- real failure ----------
            tcp.close();
            return util::ResultV<HttpResponse>::Err(err);
        }

        // 关闭连接
        tcp.close();

        // ---------------- build response ----------------
        auto res = parser.get();

        HttpResponse out;
        out.status = res.result_int();

        for (auto const &h : res.base())
            out.headers.emplace(h.name_string(), h.value());

        out.body = beast::buffers_to_string(res.body().data());

        // 构建最终的 HttpResponse 对象返回
        return util::ResultV<HttpResponse>::Ok(std::move(out));
    }
}
