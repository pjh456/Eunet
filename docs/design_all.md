# 全局架构设计与模块目录

## 1. 项目概述
**EuNet** 是一个基于 C++20 的网络请求可视化工具。它不仅仅是一个 HTTP 客户端，更是一个**可观测的网络运行时环境**。

### 核心设计理念
1.  **可观测性优先 (Observability First)**：与传统网络库不同，该库的核心目标不是极致的吞吐量，而是**暴露网络过程中的每一个状态变化**（DNS解析、TCP握手、TLS握手、数据传输、状态流转）。
2.  **事件驱动架构 (Event-Driven Architecture)**：系统的核心是“事件总线”。底层的网络操作产生 `Event`，通过 `Orchestrator`（编排器）分发给 `Timeline`（存储）和 `Sink`（UI渲染）。
3.  **Rust 风格的错误处理**：通过 `Result<T, E>` 消除异常（Exception），强制显式处理错误，并提供语义丰富的错误分类（`util::Error`）。
4.  **RAII 与 资源管理**：通过 `Fd` 类和智能指针管理文件描述符与内存，防止资源泄漏。

## 2. 模块分层架构

系统自底向上分为五层：

| 层级 | 模块名称 | 职责描述 |
| :--- | :--- | :--- |
| **L5 (UI)** | `ui/` | **用户界面层**。基于 FTXUI 库，负责将核心层的事件快照渲染为终端图形界面。 |
| **L4 (App)** | `net/` | **应用协议层**。实现 HTTP、TCP Client 等具体协议逻辑，负责产生高层业务事件。 |
| **L3 (Core)** | `core/` | **核心逻辑层**。系统的“大脑”。负责事件聚合、生命周期状态机 (FSM)、时间线记录、线程调度。 |
| **L2 (HAL)** | `platform/` | **平台抽象层**。封装 OS 系统调用（Socket, Epoll, DNS, Time, Capability），屏蔽 Linux 底层细节。 |
| **L1 (Util)** | `util/` | **基础工具层**。提供通用的 Result 类型、错误处理机制、字节缓冲区等基础设施。 |

---

## 3. 详细模块目录设计文档

以下是各目录的职责摘要，稍后将展开每个文件的详细设计。

### 3.1 [`util/` (通用工具)](./design/util_design.md)
该模块不依赖任何业务逻辑，提供纯粹的 C++ 工具类。
*   **error.hpp/cpp**: 定义了统一的错误类型 `Error`，包含域（Domain）、类别（Category）和严重程度。
*   **result.hpp**: 实现了 `Result<T, E>` 模版，用于替代异常处理，提供类似 Rust 的链式调用（`map`, `and_then`）。
*   **byte_buffer.hpp/cpp**: 提供类似 Netty 的 `ByteBuffer`，支持自动扩容、读写指针管理。

### 3.2 [`platform/` (平台适配)](./design/platform_design.md)
该模块直接与 Linux Kernel 交互。
*   **fd.hpp/cpp**: 文件描述符的 RAII 封装，确保 `close` 被调用。
*   **poller.hpp/cpp**: `epoll` 的面向对象封装，处理 IO 多路复用。
*   **time.hpp/cpp**: 统一的时间处理，区分单调时间（计时）和墙上时间（显示）。
*   **capability.hpp/cpp**: Linux Capabilities 管理（如申请 `CAP_NET_RAW`）。
*   **net/**:
    *   **base_socket.hpp/cpp**: Socket 通用基类。
    *   **socket/tcp_socket.hpp**: TCP 特定实现。
    *   **socket/udp_socket.hpp**: UDP 特定实现。
    *   **endpoint.hpp**: IP 地址与端口封装。
    *   **dns_resolver.hpp**: 域名解析封装。

### 3.3 [`core/` (核心引擎)](./design/core_design.md)
该模块不涉及具体的 Socket API，只关心数据流和状态。
*   **event.hpp/cpp**: 定义系统中所有可能发生的事件结构。
*   **event_snapshot.hpp**: 用于 UI 渲染的事件不可变快照。
*   **lifecycle_fsm.hpp/cpp**: 能够根据事件流推导连接状态（Init -> Connecting -> Established...）的状态机。
*   **timeline.hpp/cpp**: 内存中的时序数据库，支持按 FD、时间、类型查询事件。
*   **orchestrator.hpp/cpp**: 核心控制器，连接 Timeline、FSM 和 UI Sink。
*   **engine.hpp**: 负责驱动 `Scenario` 运行的线程包装器。
*   **sink.hpp**: 事件消费者的接口定义。

### 3.4 [`net/` (网络实现)](./design/net_design.md)
该模块组合 Platform 和 Core，执行实际的网络任务。
*   **connection.hpp**: 连接抽象接口。
*   **connection/tcp_connection.hpp**: 组合了 `TCPSocket` 和 `ByteBuffer`。
*   **tcp_client.hpp/cpp**: 执行 TCP 连接流程，并向 Orchestrator 发送事件。
*   **http_client.hpp/cpp**: 基于 `TCPClient` 和 `Boost.Beast` 实现 HTTP 协议解析。
*   **http_scenario.hpp/cpp**: 定义具体的 HTTP 请求任务（URL解析、执行流程）。

### 3.5 [`tui/` (用户界面)](./design/tui_design.md)
*   **tui_app.hpp**: 定义 FTXUI 的布局、渲染循环和数据绑定。
*   **tui_sink.hpp**: 核心层到 UI 层的桥梁，处理线程安全的事件投递。

