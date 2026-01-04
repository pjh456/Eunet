# 核心逻辑层 (Core)

## 第三方依赖汇总
核心层主要负责数据流转和状态管理，尽量保持零依赖，主要使用标准库。
*   **fmt**: 事件消息构建复杂，会用到 fmt 进行格式化。

## 1 `core/event.hpp` & `cpp`

**外部依赖**: 无

**设计思路**：
这是整个可视化工具的核心数据结构。所有的网络行为都必须被具体化为 Event。

**模块职责**：
定义事件的数据结构。

**实现方法**：
*   `EventType` 枚举：涵盖从 DNS 到 HTTP Body 的所有阶段。
*   `Event` 结构体：包含 `SessionId`, `Fd`, `Timestamp`, `Message` 和可选的 `Error`。
*   提供工厂方法 `info()` 和 `failure()` 方便构造。

## 2 `core/lifecycle_fsm.hpp` & `cpp`

**外部依赖**: 无

**设计思路**：
仅仅有一堆离散的 Event 是不够的，我们需要知道当前连接处于什么“状态”（State）。

**模块职责**：
根据 Event 流推导连接的生命周期状态。

**实现方法**：
*   **LifecycleFSM**: 单个连接的状态机。维护 `current_state` (Init, Resolving, Connecting, Established...)。
*   `transit()`: 状态流转函数。例如收到 `DNS_RESOLVE_START` 转入 `Resolving` 状态。
*   **FsmManager**: 管理所有 Session 的状态机集合，通过 `SessionId` 或 `Fd` 查找对应的 FSM。

## 3 `core/timeline.hpp` & `cpp`

**外部依赖**: 无

**设计思路**：
UI 需要回放历史，或者查询特定 FD 的日志。需要一个内存数据库。

**模块职责**：
存储和索引所有发生的事件。

**实现方法**：
*   `events`: `std::vector<Event>` 存储所有事件。
*   `fd_index`, `type_index`: 哈希表索引，加速查询。
*   提供线程安全的 `push` 和 `query` 接口。

## 4 `core/orchestrator.hpp` & `cpp`

**外部依赖**: 无

**设计思路**：
解耦生产者（Net Layer）和消费者（UI Layer）。

**模块职责**：
系统的中枢神经。接收事件，更新状态，分发通知。

**实现方法**：
*   持有 `Timeline` 和 `FsmManager`。
*   `emit(Event)`:
    1.  写入 Timeline。
    2.  更新 FSM。
    3.  生成 `EventSnapshot`。
    4.  遍历所有 registered `Sinks` 并调用回调。
*   持有 `sinks` 列表（观察者模式）。

## 5 `core/sink.hpp`

**外部依赖**: 无

**模块职责**：
事件消费者的接口。

**实现方法**：
*   纯虚基类 `IEventSink`，定义 `on_event(EventSnapshot)`。

## 6 `core/engine.hpp`

**外部依赖**: 无

**模块职责**：
负责在一个独立的线程中运行具体的网络场景（Scenario）。

**实现方法**：
*   持有 `std::thread`。
*   `execute()`: 启动新线程运行 Scenario，避免阻塞主 UI 线程。
