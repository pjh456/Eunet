# EuNet

![Language](https://img.shields.io/badge/language-C%2B%2B20-blue.svg)
![Platform](https://img.shields.io/badge/platform-openEuler-green.svg)
![License](https://img.shields.io/badge/license-MIT-orange.svg)

> **EuNet** 是一个基于 **openEuler** 的现代 C++ 网络请求可视化工具。
> 它聚焦于 **“让网络请求过程变得可观察”**，将黑盒的网络 IO 拆解为可视化的事件流。

EuNet 的核心目标不是“发送请求”，而是 **展示一次网络请求从开始到结束究竟发生了什么**。

---

## ✨ 核心特性

* 🔍 **全生命周期可视化**
  * 细粒度展示 DNS 解析、TCP 握手、TLS 协商、数据传输等阶段。
  * 精确捕获关键耗时指标（Resolve Time, Connect Time, TTFB 等）。
* 🧭 **事件驱动的时间线**
  * 基于 `epoll` 和状态机（FSM），以事件流形式重建请求过程。
  * 支持回放和查看特定文件描述符（FD）的历史事件。
* 🖥️ **沉浸式终端界面 (TUI)**
  * 基于 `FTXUI` 构建，无需图形界面即可在服务器终端查看结构化数据。
  * 实时状态指示（连接中、传输中、空闲、错误）。
* 🐧 **原生 Linux 深度集成**
  * 直接操作 Socket API，封装 `epoll`、Capabilities 和系统调用。
  * 采用 RAII 管理资源，配合 C++20 现代特性。

---

## 📐 架构与设计

EuNet 采用分层架构，将 UI 展示、核心编排和底层网络实现完全解耦。

```mermaid
graph TD
    UI[TUI Layer (FTXUI)] --> Sink[Event Sink]
    Sink --> Orch[Orchestrator]
    Orch --> Timeline[Timeline & FSM]
    Orch --> Net[Network Scenarios]
    Net --> Platform[Platform / HAL (Epoll, Socket)]
```

📚 **深入了解**

我们提供了详细的架构设计文档，包含模块职责、类图设计和实现细节。

👉 **[点击查看：EuNet 架构与设计文档](docs/design_all.md)**

---

## 🚀 快速开始

### 0. 一键脚本！

使用 `scripts/build.sh` 一键脚本从依赖安装到自动构建！

在项目根目录下执行以下命令：
```bash
cd scripts
bash ./build.sh
```

### 1. 环境依赖

EuNet 依赖现代 C++ 编译器和部分系统库。

*   **编译器**: GCC 10+ 或 Clang 11+ (支持 C++20)
*   **构建工具**: CMake 3.16+
*   **系统库**: `fmt`, `boost` (用于 HTTP 解析)

#### 在 openEuler 上安装:

```bash
sudo dnf install cmake gcc-c++ fmt-devel boost-devel
```

EuNet 使用 `FTXUI` 作为界面库，需要手动在 `include/` 文件夹下克隆

```bash
cd eunet
cd include
git clone https://github.com/ArthurSonzogni/FTXUI.git
```

### 2. 获取源码与构建

```bash
# 1. 克隆仓库
git clone https://gitlab.eduxiji.net/T202510423998135/project3035747-358488
cd eunet

# 2. 准备构建目录
mkdir -p build && cd build

# 3. 编译
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### 3. 运行

构建完成后，二进制文件位于 `build/` 目录下。

```bash
# 基础用法
./eunet http://www.example.com

# 指定特定端口
./eunet http://localhost:8080
```

---

## 🧩 运行效果示例

当执行 `eunet http://example.com` 时，终端界面将显示如下事件流：

```text
[00ms]  DNS_RESOLVE_START    Resolving host: example.com
[12ms]  DNS_RESOLVE_DONE     Resolved to: 93.184.216.34
[15ms]  TCP_CONNECT_START    Connecting to 93.184.216.34:80...
[23ms]  TCP_CONNECT_SUCCESS  Connection established (fd=4)
[24ms]  HTTP_REQUEST_BUILD   HTTP GET /
[26ms]  HTTP_SENT            Request sent (128 bytes)
[78ms]  HTTP_HEADERS_RCVD    Headers received (200 OK)
[120ms] HTTP_BODY_DONE       Body received (1256 bytes)
[121ms] CONNECTION_CLOSED    Closing connection
```

---

## 🛠️ 技术栈

*   **语言标准**: C++20 (Concepts, Span, Smart Pointers)
*   **构建系统**: CMake
*   **核心组件**:
    *   **UI**: [FTXUI](https://github.com/ArthurSonzogni/FTXUI) (终端界面)
    *   **Async I/O**: `epoll` (Linux 原生多路复用)
    *   **Protocol**: `Boost.Beast` (HTTP Parser), Berkeley Sockets
    *   **Utils**: `fmt` (格式化), `Result<T,E>` (错误处理)

---

## 🗓️ 开发路线图

- [x] **Phase 0: 基础设施**
    - [x] CMake 构建系统
    - [x] `Result` / `Error` 错误处理机制
    - [x] `ByteBuffer` 与 `Fd` 封装
- [x] **Phase 1: 平台抽象 (HAL)**
    - [x] `Poller` (Epoll 封装)
    - [x] `TCPSocket` / `UDPSocket`
    - [x] `DNSResolver`
- [x] **Phase 2: 核心引擎**
    - [x] 事件定义 (`Event`) 与 快照 (`EventSnapshot`)
    - [x] 生命周期状态机 (`LifecycleFSM`)
    - [x] 编排器 (`Orchestrator`)
- [x] **Phase 3: 网络实现**
    - [x] `TCPClient` (带事件上报)
    - [x] `HTTPClient` (GET 请求)
- [x] **Phase 4: 可视化 (MVP)**
    - [x] TUI 列表展示
    - [x] 实时状态刷新
- [ ] **Phase 5: 高级特性 (To-Do)**
    - [ ] HTTPS / TLS 支持 (OpenSSL 集成)
    - [ ] ICMP Ping 支持
    - [ ] 详细的统计面板 (RTT, 吞吐量)

---

## 📚 适合人群

*   希望深入理解 **Linux 网络编程** 与 **Epoll 模型** 的开发者。
*   寻找 **C++20 工程实践** 案例的学习者。
*   对 **网络可观测性 (Observability)** 和 **性能分析** 感兴趣的工程师。

---

## 📄 License

MIT License