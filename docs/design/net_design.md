# 网络协议层 (Net)

## 第三方依赖汇总
此层负责具体协议的解析与传输，重度依赖 Boost 库。
*   **Boost.Beast** (License: Boost): 工业级的 HTTP/WebSocket 协议解析与序列化实现。
*   **Boost.Asio** (License: Boost): Beast 的底层依赖（主要使用其 Buffer 概念）。
*   **fmt** (License: MIT): 用于事件日志的格式化。

## 1 `net/connection.hpp` (及 `tcp_connection.hpp`, `udp_connection.hpp`)

**设计思路**：
组合 Socket 和 Buffer，提供更高层的读写接口。

**模块职责**：
管理单条连接的 IO 和缓冲。

**实现方法**：
*   持有 `TCPSocket`/`UDPSocket`。
*   持有 `in_buffer` 和 `out_buffer`。
*   `read()`: 先读 Buffer，不够再读 Socket。
*   `write()`: 尝试直写 Socket，写不完存入 Buffer。

## 2 `net/tcp_client.hpp` & `cpp`

**设计思路**：
这是“可观测性”植入的关键地方。在执行标准的 TCP 流程时，**主动**向 `Orchestrator` 发送事件。

**模块职责**：
执行 TCP 连接逻辑，并产生事件。

**实现方法**：
*   持有 `Orchestrator&`。
*   `connect()`:
    1.  emit `DNS_RESOLVE_START`.
    2.  Call `DNSResolver`.
    3.  emit `DNS_RESOLVE_DONE`.
    4.  emit `TCP_CONNECT_START`.
    5.  Call `socket.connect`.
    6.  emit `TCP_CONNECT_SUCCESS` / `TIMEOUT`.

## 3 `net/http_client.hpp` & `cpp`

**设计思路**：
利用 `Boost.Beast` 处理 HTTP 协议细节，利用自己的 `TCPClient` 处理传输和事件上报。

**模块职责**：
HTTP 协议客户端。

**实现方法**：
*   `get(HttpRequest)`:
    1.  调用 `tcp.connect()`。
    2.  emit `HTTP_REQUEST_BUILD`.
    3.  构建 Beast Request 对象。
    4.  调用 `tcp.send()`。
    5.  emit `HTTP_SENT`.
    6.  循环 `tcp.recv()` 并喂给 `beast::http::parser`。
    7.  emit `HTTP_BODY_DONE`.

## 4 `net/http_scenario.hpp` & `cpp`

**设计思路**：
封装一次具体的任务，比如“GET baidu.com”。

**模块职责**：
解析用户输入 URL，调用 `HTTPClient`。

**实现方法**：
*   URL 解析逻辑（scheme, host, port, path）。
*   `run()` 方法：执行实际请求。
