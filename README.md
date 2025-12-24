# EuNet

> **EuNet** 是一个基于 **openEuler / Linux** 的网络请求可视化工具，
> 聚焦于 **“让网络请求过程变得可观察”**，而不是完整复刻 `curl` 或 `ping` 的全部功能。

EuNet 的核心目标不是“发请求”，而是 **展示一次网络请求从开始到结束发生了什么**。

---

## ✨ 特性概览

* 🔍 **网络请求生命周期可视化**

  * DNS 解析
  * TCP 建立连接
  * 请求发送 / 响应接收
  * 首字节时间（TTFB）、总耗时等关键节点
* 🧭 **时间线视角**

  * 以事件流形式重建一次请求的完整过程
* 🖥️ **终端友好（TUI）**

  * 无需 GUI，直接在终端中查看结构化结果
* 🐧 **面向 Linux / openEuler**

  * 使用标准 Linux 网络接口与机制
* 🧱 **模块化设计**

  * 网络引擎、事件模型、展示层解耦

---

## 🎯 设计目标

EuNet **不是**：

* 又一个功能繁杂的 `curl`
* 又一个简单输出 RTT 的 `ping`

EuNet **是**：

* 一个 **网络行为观察工具**
* 一个 **学习 Linux 网络、Socket、协议栈的工程实践**
* 一个 **把“黑盒请求”拆解为“可解释过程”的工具**

---

## 🧩 核心理念

> **网络请求不是一个函数调用，而是一连串事件。**

EuNet 将一次请求建模为**状态机 + 时间线**：

```
DNS_START
  ↓
DNS_DONE
  ↓
TCP_CONNECTING
  ↓
TCP_ESTABLISHED
  ↓
REQUEST_SENT
  ↓
RESPONSE_RECEIVING
  ↓
COMPLETED / ERROR
```

每个阶段都有明确的时间戳和状态含义。

---

## 🚀 使用示例（规划中）

```bash
eunet ping example.com
```

```
[0ms]   ICMP Echo Request sent
[18ms]  ICMP Echo Reply received
RTT: 18ms
```

```bash
eunet http https://example.com
```

```
[0ms]   DNS lookup start
[12ms]  DNS resolved (93.184.216.34)
[15ms]  TCP connect start
[23ms]  TCP established
[26ms]  HTTP request sent
[78ms]  First byte received
[120ms] Response completed (200 OK)
```

---

## 🏗️ 架构设计（概念）

```
+----------------------+
|        TUI           |
|  时间线 / 状态展示   |
+----------▲-----------+
           |
+----------+-----------+
|   Event Timeline     |
|  请求生命周期模型    |
+----------▲-----------+
           |
+----------+-----------+
|  Network Engine      |
|  Socket / ICMP / TCP |
+----------------------+
```

---

## 🛠️ 技术栈（规划）

* **平台**：Linux / openEuler
* **语言**：C / C++ / Rust（待定）
* **网络**：

  * BSD Socket
  * ICMP / TCP / HTTP
* **可视化**：

  * TUI（ncurses / tui-rs）
* **可选扩展**：

  * libpcap
  * eBPF（长期目标）

---

## 📦 构建与运行（占位）

> 当前项目处于早期阶段，构建方式将在核心功能稳定后补充。

```bash
# TODO
```

---

## 🧪 开发路线（简要）

* [ ] 最小 ICMP ping 实现
* [ ] TCP 连接 + 时间统计
* [ ] HTTP 请求生命周期拆解
* [ ] 事件时间线模型
* [ ] TUI 展示
* [ ] openEuler 适配与测试
* [ ] 文档与示例补充

---

## 📚 适合人群

* 希望深入理解 **Linux 网络栈**
* 想写一个 **真正“像系统工具”的项目**
* 对 **网络可观测性 / 性能分析**感兴趣
* 学习阶段，希望通过工程实践打基础

---

## 📄 License

MIT License
