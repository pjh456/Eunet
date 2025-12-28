#ifndef INCLUDE_EUNET_TUI_TUI_APP
#define INCLUDE_EUNET_TUI_TUI_APP

#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component.hpp"

#include <fmt/format.h>

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

        // UI 内部状态
        std::vector<core::EventSnapshot> event_logs_;
        int selected_event_idx_ = 0;
        std::mutex data_mtx_;

        std::string current_url_str_;
        Component input_url_;
        Component go_button_;
        Component input_row_;

    public:
        explicit TuiApp(core::Orchestrator &orch, core::NetworkEngine &engine)
            : orch_(orch), engine_(engine)
        {
            input_url_ = Input(&current_url_str_, "Enter URL here...");
            go_button_ = Button(" GO! ", [this]
                                {
            if (!current_url_str_.empty())
                engine_.execute(std::make_unique<net::http::HttpGetScenario>(current_url_str_)); });
            input_row_ = Container::Horizontal({input_url_, go_button_});
        }

        void run()
        {
            // 注册 Sink
            auto sink = std::make_shared<TuiSink>(screen_, [this](const auto &snap)
                                                  {
            std::lock_guard lock(data_mtx_);
            event_logs_.push_back(snap); });
            orch_.attach(sink.get());

            // 主界面组件
            auto main_ui = Renderer(input_row_, [this]
                                    {
            std::lock_guard lock(data_mtx_);
            return render_main_view(); });

            screen_.Loop(main_ui);
        }

    private:
        Element render_main_view()
        {
            return vbox(
                       {// 顶部标题栏
                        hbox({text(" EuNet - Network Request Visualizer ") | bold | color(Color::Cyan),
                              filler(),
                              text(to_string(platform::time::wall_now())) | dim}) |
                            borderLight,

                        // 中间内容：事件列表 + 详情面板
                        hbox({render_event_list() | flex_grow,
                              separator(),
                              render_details_panel() | size(WIDTH, EQUAL, 40)}) |
                            flex_grow,

                        // 底部状态栏
                        // render_status_bar(),

                        // 输入栏
                        hbox({input_url_->Render() | border | color(Color::White),
                              go_button_->Render() | border}) |
                            border}) |
                   border;
        }

        Element render_event_list()
        {
            Elements items;
            for (size_t i = 0; i < event_logs_.size(); ++i)
            {
                const auto &snap = event_logs_[i];
                Color state_color = snap.error ? Color::Red : Color::Green;

                items.push_back(hbox({text(fmt::format("[{:>3}ms] ", int(i * 10))) | dim,
                                      text(to_string(snap.state)) | color(state_color) | size(WIDTH, EQUAL, 12),
                                      separatorLight(),
                                      text(" " + snap.event.msg)}));
            }
            return vbox(std::move(items)) | vscroll_indicator | frame;
        }

        Element render_details_panel()
        {
            if (event_logs_.empty())
                return text("No events yet...") | center;

            const auto &last = event_logs_.back();
            return vbox({text(" EVENT DETAILS ") | hcenter | bold,
                         separator(),
                         window(text("FSM State"), text(to_string(last.state))),
                         window(text("FD Info"), text(fmt::format("Handle: {}", last.fd))),
                         last.error ? window(text("Error"), text(last.error.format()) | color(Color::Red))
                                    : emptyElement()});
        }
    };
}

#endif // INCLUDE_EUNET_TUI_TUI_APP