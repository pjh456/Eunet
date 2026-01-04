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

**设计思路**：
使用 FTXUI 构建终端界面。

**模块职责**：
UI 布局、渲染、交互处理。

**实现方法**：
*   **Layout**: 左侧列表（Menu），右侧详情（Detail Panel），顶部状态栏。
*   **Data Binding**: 维护一个 `snapshots_` 列表。
*   **Concurrency**: 使用 `std::mutex` 保护数据，因为 `on_event` 在网络线程回调，而 `Render` 在 UI 线程。
*   **Render Logic**: 根据 Event 状态（Error 红色，Success 绿色）动态生成 UI 元素。

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