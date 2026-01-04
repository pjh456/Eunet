/*
 * ============================================================================
 *  File Name   : benchmark_test.cpp
 *  Module      : test
 *
 *  Description :
 *      性能基准测试套件。
 *      横向对比 LibCurl (C), Boost.Beast (C++) 与 EuNet 的 HTTP GET 性能。
 *
 *  Metrics :
 *      - RPS (Requests Per Second)
 *      - CPU Usage (User/System)
 *      - Memory Footprint (Max RSS)
 *
 *  Notes :
 *      为了公平对比，所有客户端均配置为：
 *      1. 关闭代理 (No Proxy)
 *      2. 开启 TCP_NODELAY (禁用 Nagle 算法)
 *      3. 使用 Keep-Alive (长连接复用)
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-5
 *
 * ============================================================================
 */

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <curl/curl.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <iomanip>

// 资源监控头文件 (Linux/macOS)
#include <sys/resource.h>
#include <unistd.h>

#define ENABLE_EUNET

#ifdef ENABLE_EUNET
#include "eunet/core/orchestrator.hpp"
#include "eunet/net/http_client.hpp"
#endif

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

// ================= 配置参数 =================
static constexpr const char *RESPONSE_BODY = "0123456789abcdef0123456789abcdef\n";
constexpr const char *HOST = "127.0.0.1";
constexpr const char *PATH = "/test_small.txt";
constexpr int WARMUP_REQUESTS = 100; // 预热请求数
constexpr int BENCH_REQUESTS = 1000; // 正式测试请求数

// ================= 资源监控工具类 =================
class Profiler
{
public:
    Profiler(std::string name) : name_(std::move(name))
    {
        reset();
    }

    void start()
    {
        getrusage(RUSAGE_SELF, &start_usage_);
        start_time_ = std::chrono::steady_clock::now();
    }

    void stop(int success_count, int total_requests)
    {
        auto end_time = std::chrono::steady_clock::now();
        struct rusage end_usage;
        getrusage(RUSAGE_SELF, &end_usage);

        long wall_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_).count();
        if (wall_ms == 0)
            wall_ms = 1; // 防止除零

        // 计算 CPU 时间 (User + System)
        double user_cpu_ms = (end_usage.ru_utime.tv_sec - start_usage_.ru_utime.tv_sec) * 1000.0 +
                             (end_usage.ru_utime.tv_usec - start_usage_.ru_utime.tv_usec) / 1000.0;
        double sys_cpu_ms = (end_usage.ru_stime.tv_sec - start_usage_.ru_stime.tv_sec) * 1000.0 +
                            (end_usage.ru_stime.tv_usec - start_usage_.ru_stime.tv_usec) / 1000.0;

        // 内存 (Max RSS) - Linux下单位通常是 KB
        long mem_kb = end_usage.ru_maxrss;

        double rps = total_requests * 1000.0 / wall_ms;

        std::cout << "------------------------------------------------------------\n";
        std::cout << "[" << name_ << "] Result:\n";
        std::cout << "  Requests : " << total_requests << " (Success: " << success_count << ")\n";
        std::cout << "  Time     : " << wall_ms << " ms\n";
        std::cout << "  RPS      : " << std::fixed << std::setprecision(1) << rps << " req/s\n";
        std::cout << "  CPU User : " << user_cpu_ms << " ms\n";
        std::cout << "  CPU Sys  : " << sys_cpu_ms << " ms\n";
        std::cout << "  CPU Load : " << std::setprecision(1) << ((user_cpu_ms + sys_cpu_ms) / wall_ms * 100.0) << " %\n";
        std::cout << "  Mem Peak : " << mem_kb / 1024.0 << " MB (Max RSS)\n";
        std::cout << "------------------------------------------------------------\n";
    }

private:
    void reset()
    {
        start_time_ = std::chrono::steady_clock::now();
    }

    std::string name_;
    std::chrono::steady_clock::time_point start_time_;
    struct rusage start_usage_;
};

// ================= 服务器实现 (保持原样，稍作清理) =================
void handle_session(tcp::socket socket)
{
    beast::error_code ec;
    beast::flat_buffer buffer;
    for (;;)
    {
        http::request<http::empty_body> req;
        http::read(socket, buffer, req, ec);
        if (ec == http::error::end_of_stream)
            break;
        if (ec)
            break;

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "BenchServer");
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = RESPONSE_BODY;
        res.prepare_payload();

        http::write(socket, res, ec);
        if (ec)
            break;
        if (!res.keep_alive())
        {
            break;
        }
    }
    socket.shutdown(tcp::socket::shutdown_both, ec);
}

class BenchServer
{
public:
    explicit BenchServer(uint16_t port) : ioc_(1), acceptor_(ioc_)
    {
        tcp::endpoint ep(tcp::v4(), port);
        acceptor_.open(ep.protocol());
        acceptor_.set_option(asio::socket_base::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();
        thread_ = std::thread([this]
                              { run(); });
        thread_.detach();
    }
    uint16_t port() const { return acceptor_.local_endpoint().port(); }

private:
    void run()
    {
        while (true)
        {
            tcp::socket socket(ioc_);
            beast::error_code ec;
            acceptor_.accept(socket, ec);
            if (!ec)
                std::thread(&handle_session, std::move(socket)).detach();
        }
    }
    asio::io_context ioc_;
    tcp::acceptor acceptor_;
    std::thread thread_;
};

// ================= LibCurl 评测 =================
static size_t discard_cb(void *, size_t size, size_t nmemb, void *)
{
    return size * nmemb;
}

void bench_libcurl_reuse(uint16_t port, int requests)
{
    Profiler prof("LibCurl Reuse");

    CURL *curl = curl_easy_init();
    std::string url = std::string("http://") + HOST + ":" + std::to_string(port) + PATH;

    // 基础设置
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000L);

    // 【关键修正1】禁用代理，防止 502 Bad Gateway
    curl_easy_setopt(curl, CURLOPT_PROXY, "");
    curl_easy_setopt(curl, CURLOPT_NOPROXY, "*");

    // 【关键修正2】启用 TCP_NODELAY，对 benchmark 极其重要
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

    // 【关键修正3】移除 CURLOPT_FORBID_REUSE，允许长连接

    // 预热 (不计入统计)
    for (int i = 0; i < WARMUP_REQUESTS; ++i)
        curl_easy_perform(curl);

    // 开始测试
    prof.start();
    int success = 0;
    for (int i = 0; i < requests; ++i)
    {
        CURLcode rc = curl_easy_perform(curl);
        if (rc == CURLE_OK)
        {
            long code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
            if (code == 200)
                success++;
        }
    }
    prof.stop(success, requests);

    curl_easy_cleanup(curl);
}

// ================= Beast 评测 =================
void bench_beast_reuse(uint16_t port, int requests)
{
    Profiler prof("Boost.Beast Reuse");
    asio::io_context ioc;
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    auto endpoints = resolver.resolve(HOST, std::to_string(port));
    stream.connect(endpoints);
    stream.expires_never();

    // 启用 TCP NoDelay
    stream.socket().set_option(tcp::no_delay(true));

    // 预备 Request 对象
    http::request<http::empty_body> req{http::verb::get, PATH, 11};
    req.set(http::field::host, HOST);
    req.keep_alive(true);

    // 预热
    beast::flat_buffer buffer;
    for (int i = 0; i < WARMUP_REQUESTS; ++i)
    {
        http::write(stream, req);
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);
        buffer.consume(buffer.size()); // 清空 buffer
    }

    // 开始测试
    prof.start();
    int success = 0;
    for (int i = 0; i < requests; ++i)
    {
        http::write(stream, req);

        http::response<http::dynamic_body> res; // 每次重新构造 response 比较公平，或者 clear
        http::read(stream, buffer, res);

        if (res.result() == http::status::ok)
            success++;

        // 重要：Beast flat_buffer 需要管理，但 read 通常会自动处理扩容，
        // 为了防止内存无限增长，最好在这里 consume 掉
        // buffer.consume(buffer.size());
    }
    prof.stop(success, requests);

    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
}

// ================= Eunet 评测 =================
void bench_eunet(uint16_t port, int requests)
{
#ifdef ENABLE_EUNET
    Profiler prof("Eunet Framework");
    core::Orchestrator orch;
    net::http::HTTPClient client(orch);

    // 预热
    for (int i = 0; i < WARMUP_REQUESTS; ++i)
    {
        client.get({.host = HOST, .port = port, .target = PATH});
    }

    prof.start();
    int success = 0;
    for (int i = 0; i < requests; ++i)
    {
        // 注意：eunet 内部无法复用连接
        auto res = client.get(
            {.host = HOST,
             .port = port,
             .target = PATH,
             .timeout_ms = 3000,
             .connection_close = true});

        if (res.is_ok() && res.unwrap().status == 200)
            success++;
    }
    prof.stop(success, requests);
#else
    std::cout << "[Eunet] Skipped (ENABLE_EUNET not defined)\n";
#endif
}

// ================= Main =================
int main()
{
    // 初始化全局 curl (只做一次)
    curl_global_init(CURL_GLOBAL_ALL);

    std::cout << "Starting benchmark server on random port..." << std::endl;
    BenchServer server(0);
    uint16_t port = server.port();
    std::cout << "Server listening on port: " << port << std::endl;
    std::cout << "Requests per test: " << BENCH_REQUESTS << std::endl;

    // 给服务器一点时间启动
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\n=== Running Benchmarks ===\n"
              << std::endl;

    bench_libcurl_reuse(port, BENCH_REQUESTS);

    // 给系统一点喘息时间，回收端口等
    std::this_thread::sleep_for(std::chrono::seconds(1));

    bench_beast_reuse(port, BENCH_REQUESTS);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    bench_eunet(port, BENCH_REQUESTS);

    std::cout << "Benchmark finished." << std::endl;

    curl_global_cleanup();
    return 0;
}