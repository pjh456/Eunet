/*
 * ============================================================================
 *  File Name   : http_response.hpp
 *  Module      : net/http
 *
 *  Description :
 *      HTTP 响应的数据结构定义。包含状态码、状态原因、响应头
 *      和响应体。提供简便的 header 查找和状态判断方法。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_NET_HTTP_RESPONSE
#define INCLUDE_EUNET_NET_HTTP_RESPONSE

#include <string>
#include <map>

namespace net::http
{
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
}

#endif // INCLUDE_EUNET_NET_HTTP_RESPONSE