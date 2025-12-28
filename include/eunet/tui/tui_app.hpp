#ifndef INCLUDE_EUNET_TUI_TUI_APP
#define INCLUDE_EUNET_TUI_TUI_APP

#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component.hpp"

#include <fmt/format.h>

#include "eunet/core/orchestrator.hpp"
#include "eunet/tui/tui_sink.hpp"

namespace ui
{
    using namespace ftxui;

    class TuiApp
    {
    private:
        core::Orchestrator &orch_;
        ScreenInteractive screen_ = ScreenInteractive::Fullscreen();

        // UI 内部状态
        std::vector<core::EventSnapshot> event_logs_;
        int selected_event_idx_ = 0;
        std::mutex data_mtx_;

    public:
        explicit TuiApp(core::Orchestrator &orch) : orch_(orch) {}

        void run()
        {
            // 1. 注册 Sink
            auto sink = std::make_shared<
                TuiSink>(
                screen_,
                [this](const auto &snap)
                {
                    std::lock_guard lock(data_mtx_);
                    event_logs_.push_back(snap);
                });
            orch_.attach(sink.get());

            // 2. 构建 UI 组件
            // auto component = Renderer([&]
            //                           {
            //     std::lock_guard lock(data_mtx_);
            //     return render_main_view(); });
            auto component = CatchEvent(
                Renderer(
                    [&]
                    {
                        std::lock_guard lock(data_mtx_);
                        return render_main_view();
                    }),
                [this](ftxui::Event event)
                {
                    if (event == ftxui::Event::Character('q') ||
                        event == ftxui::Event::Character('Q'))
                    {
                        screen_.ExitLoopClosure()(); // 退出主循环
                        return true;                 // 表示事件被处理
                    }
                    if (event == ftxui::Event::Character('s') ||
                        event == ftxui::Event::Character('S'))
                    {
                        // save_log(); // 你需要实现 save_log()
                        return true;
                    }
                    return false; // 未处理事件
                });

            // 3. 开始 FTXUI 循环
            screen_.Loop(component);
        }

    private:
        // 主绘图逻辑
        Element render_main_view()
        {
            return vbox(
                       {// 顶部标题栏
                        hbox(
                            {text(" EuNet - Network Request Visualizer ") | bold | color(Color::Cyan),
                             filler(),
                             text(to_string(platform::time::wall_now())) | dim}) |
                            borderLight,

                        // 中间主体：左侧列表 + 右侧详情
                        hbox({render_event_list() | flex_grow,
                              separator(),
                              render_details_panel() | size(WIDTH, EQUAL, 40)}) |
                            flex_grow,

                        // 底部状态栏
                        render_status_bar()}) |
                   border;
        }

        Element render_event_list()
        {
            Elements items;
            for (int i = 0; i < event_logs_.size(); ++i)
            {
                auto &snap = event_logs_[i];
                Color state_color = snap.has_error ? Color::Red : Color::Green;

                items.push_back(hbox({text(fmt::format("[{:>3}ms] ", i * 10 /* 示例时间 */)) | dim,
                                      text(to_string(snap.state)) | color(state_color) | size(WIDTH, EQUAL, 12),
                                      separatorLight(),
                                      text(" " + snap.event->msg)}));
            }
            return vbox(std::move(items)) | vscroll_indicator | frame;
        }

        Element render_details_panel()
        {
            if (event_logs_.empty())
                return text("No events yet...") | center;

            auto &last = event_logs_.back();
            return vbox({text(" EVENT DETAILS ") | hcenter | bold,
                         separator(),
                         window(text("FSM State"), text(to_string(last.state))),
                         window(text("FD Info"), text(fmt::format("Handle: {}", last.fd))),
                         last.has_error ? window(text("Error"), text(last.error.format()) | color(Color::Red))
                                        : emptyElement()});
        }

        Element render_status_bar()
        {
            return hbox({text(" [Q] Quit | [S] Save Log "),
                         filler(),
                         text(" Status: " + std::string(orch_.get_timeline().size() > 0 ? "Active" : "Idle")) | color(Color::Yellow)}) |
                   bgcolor(Color::GrayDark);
        }
    };
}

#endif // INCLUDE_EUNET_TUI_TUI_APP