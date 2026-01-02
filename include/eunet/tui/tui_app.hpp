#ifndef INCLUDE_EUNET_TUI_TUI_APP
#define INCLUDE_EUNET_TUI_TUI_APP

#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp" // 确保包含 ScreenInteractive

#include <fmt/format.h>
#include <deque>

#include "eunet/core/orchestrator.hpp"
#include "eunet/tui/tui_sink.hpp"
#include "eunet/core/engine.hpp"
#include "eunet/net/http_scenario.hpp"

namespace ui
{
    using namespace ftxui;

    class TuiApp
    {
    private:
        core::Orchestrator &orch_;
        core::NetworkEngine &engine_;
        ScreenInteractive screen_ = ScreenInteractive::Fullscreen();

        // --- 数据层 ---
        std::mutex data_mtx_;
        std::vector<core::EventSnapshot> snapshots_; // 存储完整的事件快照
        std::vector<std::string> menu_entries_;      // 存储用于 Menu 显示的字符串

        // --- UI 组件状态 ---
        int selected_menu_idx_ = 0; // 当前选中的事件索引
        std::string current_url_str_ = "http://www.baidu.com";

        // --- 组件 ---
        Component input_url_;
        Component btn_go_;
        Component event_menu_; // 事件列表菜单
        Component layout_;     // 整体布局

    public:
        explicit TuiApp(core::Orchestrator &orch, core::NetworkEngine &engine)
            : orch_(orch), engine_(engine)
        {
            init_components();
        }

        void run()
        {
            // 1. 注册 Sink (负责接收数据)
            // 注意：Sink 运行在工作线程，on_event 会并发调用
            auto sink = std::make_shared<TuiSink>(screen_, [this](const auto &snap)
                                                  { this->on_new_event(snap); });
            orch_.attach(sink);

            // 2. 启动事件循环
            // Render 逻辑运行在主线程
            screen_.Loop(layout_);
        }

    private:
        void init_components()
        {
            // 1. 输入框
            input_url_ = Input(&current_url_str_, "Enter URL...");

            // 2. 按钮：点击后清空日志并开始新请求
            btn_go_ = Button(" GO! ", [this]
                             {
                if (current_url_str_.empty()) return;
                
                // 如果引擎正在跑，简单处理：禁止重入（或者引擎本身也会处理）
                if (engine_.is_running()) return;

                this->reset_session(); // 关键：清空上一轮数据
                engine_.execute(std::make_unique<net::http::HttpGetScenario>(current_url_str_)); });

            // 3. 事件列表菜单
            // Menu 将自动绑定 menu_entries_ 和 selected_menu_idx_
            auto menu_option = MenuOption();
            menu_option.on_enter = [this] { /* 可扩展：回车查看详情弹窗等 */ };

            event_menu_ = Menu(&menu_entries_, &selected_menu_idx_, menu_option);

            // 4. 组装布局 (Container)
            // 垂直布局：[输入栏] -> [左右分栏]
            auto input_area = Container::Horizontal({input_url_,
                                                     btn_go_});

            auto content_area = Container::Horizontal({
                event_menu_ | flex, // 左侧列表自适应
                // 右侧详情不是交互组件，只是渲染视图，所以不需要放入 Container，在 Renderer 里画即可
            });

            auto main_container = Container::Vertical({input_area,
                                                       content_area});

            // 5. 渲染器 (Renderer)
            // 负责把组件的状态画出来
            layout_ = Renderer(main_container, [this]
                               {
                std::lock_guard lock(data_mtx_); // 渲染时加锁，防止数据正在写入
                return render_root(); });
        }

        // --- 逻辑处理 ---

        void on_new_event(const core::EventSnapshot &snap)
        {
            std::lock_guard lock(data_mtx_);

            snapshots_.push_back(snap);

            // 格式化菜单显示的简略信息
            std::string entry_str = fmt::format(
                "[{:>3}] {:<12} {}",
                snapshots_.size(),
                to_string(snap.state),       // 假设你有这个转换函数
                snap.event.msg.substr(0, 50) // 截断过长消息防止 UI 错乱
            );

            // 如果出错，加个标记
            if (snap.error)
            {
                entry_str = "(!)" + entry_str;
            }

            menu_entries_.push_back(entry_str);

            // 自动滚动：如果用户当前盯着最后一条，就自动选中最新的一条
            // 否则用户可能正在查看旧日志，不要打扰他
            if (selected_menu_idx_ == (int)menu_entries_.size() - 2)
            {
                selected_menu_idx_ = menu_entries_.size() - 1;
            }
        }

        void reset_session()
        {
            std::lock_guard lock(data_mtx_);
            snapshots_.clear();
            menu_entries_.clear();
            selected_menu_idx_ = 0;
            // 可以在这里加一条 "Started..." 的初始日志
        }

        // --- 渲染视图 ---

        Element render_root()
        {
            return vbox({render_header(),
                         separator(),
                         hbox({
                             render_list_panel() | flex,
                             separator(),
                             render_detail_panel() | size(WIDTH, EQUAL, 50) // 固定右侧宽度
                         }) | flex,
                         separator(),
                         render_input_bar()}) |
                   border;
        }

        Element render_header()
        {
            return hbox({text(" EuNet Visualizer ") | bold | color(Color::Cyan),
                         filler(),
                         text(engine_.is_running() ? " RUNNING " : " IDLE ") |
                             bgcolor(engine_.is_running() ? Color::Green : Color::GrayDark) | color(Color::Black)});
        }

        Element render_input_bar()
        {
            return hbox({text(" URL: "),
                         input_url_->Render() | flex,
                         btn_go_->Render()});
        }

        Element render_list_panel()
        {
            if (menu_entries_.empty())
            {
                return text("Waiting for request...") | center;
            }
            // 使用 vscroll_indicator 让菜单可滚动
            return event_menu_->Render() | vscroll_indicator | frame;
        }

        Element render_detail_panel()
        {
            if (snapshots_.empty() || selected_menu_idx_ < 0 || selected_menu_idx_ >= (int)snapshots_.size())
            {
                return text("Select an event to view details") | center;
            }

            // 获取当前选中的快照
            const auto &snap = snapshots_[selected_menu_idx_];

            // 动态构建详情视图
            Elements detail_lines;
            detail_lines.push_back(text("DETAIL INSPECTOR") | bold | hcenter);
            detail_lines.push_back(separator());

            detail_lines.push_back(hbox(text("Time: "), text(to_string(snap.ts))));
            detail_lines.push_back(hbox(text("FD:   "), text(std::to_string(snap.fd))));
            detail_lines.push_back(hbox(text("State:"), text(to_string(snap.state)) | color(Color::Yellow)));

            detail_lines.push_back(separator());
            detail_lines.push_back(text("Message:") | bold);
            detail_lines.push_back(paragraph(snap.event.msg));

            if (snap.error)
            {
                detail_lines.push_back(separator());
                detail_lines.push_back(text("ERROR INFO") | color(Color::Red) | bold);
                detail_lines.push_back(text(snap.error.format()) | color(Color::Red));
            }

            return vbox(std::move(detail_lines));
        }
    };
}

#endif // INCLUDE_EUNET_TUI_TUI_APP