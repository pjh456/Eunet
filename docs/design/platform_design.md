# 平台抽象层 (Platform)

## 1 `platform/fd.hpp` & `fd.cpp`

**设计思路**：
Linux 文件描述符是整数，容易忘记 `close`。RAII（资源获取即初始化）是 C++ 管理资源的黄金法则。

**模块职责**：
独占式管理文件描述符的生命周期。

**实现方法**：
*   **Move Only**: 禁止拷贝构造，只允许移动构造（`std::move`），确保一个 FD 只有一个所有者。
*   析构函数中自动调用 `close()`。
*   提供 `FdView`：非拥有权的弱引用，用于传参给 `epoll` 等函数，避免所有权转移。

## 2 `platform/poller.hpp` & `poller.cpp`

**设计思路**：
封装 `epoll` 的复杂性，提供面向对象的接口。

**模块职责**：
IO 多路复用器的管理。

**实现方法**：
*   持有 `epoll_fd`。
*   `add`, `modify`, `remove` 方法封装 `epoll_ctl`。
*   `wait(timeout)` 封装 `epoll_wait`，返回 `PollEvent` 结构体向量。

## 3 `platform/base_socket.hpp` & `base_socket.cpp`

**设计思路**：
提取 TCP 和 UDP 共有的逻辑（绑定 FD，获取本地/远程地址）。

**模块职责**：
Socket 的抽象基类。

**实现方法**：
*   持有 `Fd` 和 `Poller&` 的引用。
*   实现 `local_endpoint()` 和 `remote_endpoint()`（封装 `getsockname`, `getpeername`）。
*   提供 `wait_fd_epoll` 辅助函数，用于实现带超时的阻塞等待。

## 4 `platform/socket/tcp_socket.hpp` & `cpp`

**设计思路**：
实现 TCP 特有的流式读写。

**模块职责**：
TCP Socket 的具体操作。

**实现方法**：
*   `read()`: 循环 `recv` 直到 `EAGAIN`，配合 `ByteBuffer`。
*   `write()`: 循环 `send` 直到 `EAGAIN`。
*   `connect()`: 处理非阻塞 `connect` 的复杂逻辑（`EINPROGRESS` -> `epoll_wait` -> `getsockopt` 检查错误）。

## 5 `platform/socket/udp_socket.hpp` & `cpp`

**设计思路**：
UDP 是数据报，读写逻辑与 TCP 不同（不保证顺序，无连接状态）。

**模块职责**：
UDP Socket 的具体操作。

**实现方法**：
*   `connect()`: 在 UDP 中调用 `connect` 只是为了设置默认的目标地址，不进行握手。

## 6 `platform/net/dns_resolver.hpp` & `cpp`

**设计思路**：
将域名转换为 IP 地址。

**模块职责**：
DNS 解析。

**实现方法**：
*   封装 `getaddrinfo`。
*   目前是同步阻塞实现（生产环境通常需要异步 DNS，如 c-ares，但此处为简化暂用同步）。
*   将 `addrinfo` 链表转换为 `std::vector<Endpoint>`。

## 7 `platform/time.hpp` & `cpp`

**设计思路**：
区分“经过了多久”（Monotonic）和“现在几点”（Wall/System）。

**模块职责**：
提供时间点和时间段的计算。

**实现方法**：
*   `MonoClock`: 用于超时判断 (`std::chrono::steady_clock`)。
*   `WallClock`: 用于日志记录和 Event 时间戳 (`std::chrono::system_clock`)。