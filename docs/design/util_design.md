# 基础工具层 (Util)

## 1 `util/error.hpp` & `error.cpp`

**设计思路**：
传统的 `errno` 或 `std::exception` 携带信息太少。我们需要一个结构化的错误对象，能够指导业务逻辑（例如：是网络断了还是证书错了？应该重试还是报错？）。

**模块职责**：
定义全系统通用的错误载体。

**实现方法**：
*   **ErrorDomain**: 错误发生的领域（DNS, Transport, Security 等）。
*   **ErrorCategory**: 错误的语义（Timeout, Refused, AuthFailed），这是判断是否重试的关键。
*   **ErrorSeverity**: 严重程度（Fatal, Transient）。
*   **ErrorBuilder**: 链式调用的构造器模式，方便构建复杂错误。
*   **Nested Errors**: 支持 `wrap(cause)`，形成错误链，保留原始上下文。

## 2 `util/result.hpp`

**设计思路**：
C++ 异常在高性能或嵌入式场景受限，且不够显式。采用 Monadic 编程风格（类似 Rust/Swift/C++23 expected）能让错误处理路径极其清晰。

**模块职责**：
提供 `Result<T, E>` 类型，强制调用者检查返回值。

**实现方法**：
*   使用 `std::variant<T, E>` 存储结果或错误。
*   提供 `map`, `map_err`, `and_then` 等高阶函数，允许在不拆包的情况下转换数据。
*   `unwrap()` 方法在错误时抛出异常，防止未检查的访问。

## 3 `util/byte_buffer.hpp` & `byte_buffer.cpp`

**设计思路**：
裸指针 `char*` 操作容易越界。需要一个管理读写指针的动态缓冲区。

**模块职责**：
封装内存操作，提供安全的字节流读写。

**实现方法**：
*   基于 `std::vector<std::byte>` 作为底层存储。
*   维护 `read_pos` 和 `write_pos`。
*   **关键特性**:
    *   `prepare(n)` / `commit(n)`: 两阶段写入，防止写入溢出。
    *   `compact()`: 当读取位置过半时，将剩余数据移到头部，防止无限扩容。