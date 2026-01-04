# Changelog

本文件基于 Git commit 历史自动整理，按时间倒序排列。

---

## 2026-01-05

- docs: 更新了实机演示图
- feat: 更新了 HTTP 头的解析字符串
- refactor: 移除了关闭连接后的 HTTP 消息
- refactor: 移除了占位文件
- docs: 更新了 TUI 的代码注释
- docs: 更新了 TUI 设计文档
- docs: 更新了项目文档
- docs: 添加了 benchmark 测试的代码注释
- feat: 为十六进制解码页提供滚动条
- fix: 修复了右侧数据栏随解码一起滚动的问题
- fix: 修复了事件滚动导致右侧焦点改变的问题
- fix: 修复了事件快速滚动产生的横向滚动偏移
- feat: 更新了十六进制解码器

---

## 2026-01-04

- docs: 更新了项目实机演示图
- fix: 修复了偶发 need more 报错的问题
- feat: 添加了输入框，允许了重复请求
- refactor: 将 TUI 声明与实现拆分
- fix: 修复了对端关闭发送错误信息的问题
- docs: 更新了项目文档性能测试结果及分析
- docs: 更新了开发文档性能测试结果及分析
- test: 添加标准 benchmark 测试
- test: 单元测试引入了 PThreads
- docs: 添加演示视频链接
- test: 单元测试引入了 curl 库
- feat: 新增了更详细的访问请求配置
- feat: 新增了依赖目录
- docs: 为项目文档添加了演示图
- docs: 更新了设计文档的文件第三方库依赖
- docs: 更新了设计文档的第三方依赖汇总
- feat: 引入了必要的构建依赖
- docs: 更新了文件开头的文件信息等注释
- docs: 更新了项目文档
- docs: 更新了设计文档
- docs: 修复了设计文档索引问题
- docs: 更新了设计文档索引
- feat: 更新了一键构建脚本
- test: 更新 Result 单元测试
- docs: 更新了 TUI 的代码注释
- docs: 更新了 HTTP 客户端的代码注释
- docs: 更新了 TCP 客户端的代码注释
- docs: 更新了生命周期状态机和编排器的代码注释
- docs: 更新了核心事件的代码注释
- docs: 更新了 TCP 套接字的代码注释
- docs: 更新了文件描述符和事件队列的代码注释
- docs: 更新了字节缓冲区的代码注释
- docs: 更新了 Result 的代码注释
- docs: 更新了错误模型的代码注释
- docs: 更新了项目文档
- docs: 更新了核心层、网络层和终端层设计文档
- docs: 更新了工具层、平台层和全局设计文档
- fix: 修复了 TUI 代码有误的问题
- feat: 允许了请求终止后的事件调度
- feat: 为对端关闭报错开洞
- feat: 事件快照报错成为可选项
- feat: 事件报错改为可选项
- feat: 细化了 Orchestrator 报错事件
- feat: 更新了状态机状态与转移
- refactor: 移除了不需要的头文件依赖
- feat: 共享了连接之间的消息队列

---

## 2026-01-03

- docs: 更新了实机演示图
- docs: format CHANGELOG to unified heading structure
- docs: add project changelog based on commit history
- docs: 更新了项目文档构建教程
- refactor: 将 TCP 客户端等待时间降至 3000 ms
- feat: 完善了 TCP 客户端消息输出
- refactor: 重构了 HTTP 相关数据结构路径
- fix: 明确了 TCP 客户端的阻塞语义
- fix: 修复了传输结束后对端不关闭的问题
- feat: 引入 Boost 并更新 HTTP 请求实现
- feat: 更新 CMake 配置，准备导入 Boost 库
- refactor: 更改了对端关闭的错误信息
- fix: 修复了报错信息越界问题
- feat: 美化页面布局，提供更详细信息
- fix: 修复了命令行无参数无法启动的问题

---

## 2026-01-02

- test: 恢复 UDP 连接单元测试
- refactor: 恢复 UDP 连接实现
- feat: 为 TCP 连接提供了套接字访问接口
- fix: 修复套接字内部计数越界问题
- test: 恢复 TCP 连接单元测试
- refactor: 恢复 TCP 连接实现
- refactor: 重构网络连接基类以适配套接字

---

## 2026-01-01

- refactor: UDP 套接字只支持阻塞版本
- test: UDP 套接字测试通过
- refactor: 套接字如今只支持阻塞版本
- refactor: 暂时注释了 UDP 套接字相关实现
- refactor: 移除了 IO 协程上下文
- feat: 恢复 UDP 套接字实现
- fix: 修复消息队列的 fd 多次添加边界异常
- test: 提高了 TCP 套接字测试超时时限
- refactor: 将获取对端的逻辑提升至套接字基类
- test: TCP 套接字与 IO 协程上下文协作测试通过
- test: 单元测试对接新版消息队列接口测试通过
- feat: 进一步完善了统一 IO 协程上下文
- refactor: 移除了 TCP 套接字的阻塞接口
- refactor: 异步 TCP 客户端接口对接新版消息队列
- feat: 补充了消息队列的事件检查接口
- refactor: 移除套接字基类阻塞连接接口
- feat: 添加了对端主动关闭的报错语义
- refactor: 为消息队列提供 FdView 接口并修复事件检验问题
- test: 恢复 TCP 套接字单元测试
- feat: 添加 TCP 套接字阻塞接口并修复缓冲区漏洞
- feat: 新增套接字连接对端接口
- feat: 完善了消息队列枚举类型
- feat: 新增了错误类型分类获取接口
- refactor: 移除了连接对旧读写缓冲区的依赖
- feat: 为字节缓冲区提供更弱检查接口
- test: DNS 解析测试通过
- feat: 新版 DNS 解析和构造封装完成
- test: 字节缓冲区测试通过
- fix: 修复了字节缓冲区写逻辑异常的问题
- feat: 添加读写协程上下文
- refactor: 重命名套接字基类并修改基础接口
- refactor: 为重构时允许构建暂时注释部分代码
- feat: 改变部分网络职责并拆分对端解析逻辑
- refactor: 显式允许了报错的拷贝和移动构造
- feat: 添加字节缓冲区

---

## 2025-12-31

- test: 更新单元测试并舍弃部分暂无支持特性
- refactor: 统一错误系统并工程化
- refactor: 暂时移除了对同步 TCP 客户端的支持
- test: 更新了单元测试
- fix: 初步修复了文件复用的记录重叠问题
- fix: 修复了网络引擎多线程不安全的问题
- fix: 修复了 Orchestrator 的删除组件问题

---

## 2025-12-25 ～ 2025-12-30

- 项目初始化
- CMake 框架搭建
- 时间、文件描述符、事件队列、Result、权限系统、网络事件等基础封装
- TCP / UDP / Socket / Buffer / 状态机等核心网络组件逐步完成
- 单元测试体系持续完善
