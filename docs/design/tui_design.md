# 用户界面层 (UI)

## 第三方依赖汇总
UI 层高度依赖渲染库。
*   **FTXUI** (License: MIT): 用于构建 TUI（Terminal User Interface），提供组件、布局和渲染循环。
*   **fmt** (License: MIT): 用于界面上文本的格式化显示。

## 1 `tui_sink.hpp`

**外部依赖**: `FTXUI` (依赖 `ftxui::ScreenInteractive::PostEvent`)

**设计思路**：
UI 线程和网络线程是分离的。FTXUI 不是线程安全的。Sink 需要一种机制通知 UI 刷新。

**模块职责**：
将核心层事件转发到 UI 层。

**实现方法**：
*   实现 `IEventSink`。
*   `on_event`:
    1.  调用回调更新 UI 数据模型。
    2.  调用 `screen_.PostEvent(Event::Custom)` 唤醒 UI 线程重绘。

## 2 `tui_app.hpp`

**外部依赖**: `FTXUI` (Menu, Renderer, Component, Screen), `fmt`

**设计目标**:
除了展示元数据（FD, State, Error），重点解决**长 Payload 数据的可视化**问题。

**模块职责**：
UI 布局、渲染、交互处理。

**实现逻辑**:
1.  **Hex Dump 格式化**:
    *   将 `std::vector<std::byte>` 格式化为 `Offset | Hex Bytes (16) | ASCII` 的经典三段式布局。
    *   对于不可打印字符，在 ASCII 区域替换为 `.`。
2.  **虚拟滚动 (Virtual Scrolling)**:
    *   由于 FTXUI 的 `vbox` 渲染大量元素会有性能瓶颈，且终端高度有限。
    *   维护 `payload_scroll_` 状态，仅截取当前可视窗口内的行进行渲染 (`focusPositionRelative` 配合 slice)。
3.  **鼠标交互**:
    *   使用 `CatchEvent` 捕获 `Mouse::WheelUp` 和 `WheelDown`。
    *   仅当鼠标指针悬停在详情面板区域时响应滚动事件，避免干扰左侧菜单。

## 3 `main.cpp`

**外部依赖**: `FTXUI` (App loop), `fmt` (Console output fallback)

**模块职责**：
程序入口。

**实现方法**：
1.  解析命令行参数（URL）。
2.  创建 `Orchestrator`。
3.  创建 `TuiApp`。
4.  创建 `NetworkEngine`。
5.  `engine.execute(scenario)` 启动网络任务。
6.  `app.run()` 启动 UI 循环（阻塞主线程）。