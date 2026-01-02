#ifndef INCLUDE_EUNET_TUI_TUI_APP
#define INCLUDE_EUNET_TUI_TUI_APP

#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"

#include <fmt/format.h>
#include <mutex>
#include <vector>

#include "eunet/core/orchestrator.hpp"
#include "eunet/tui/tui_sink.hpp"
#include "eunet/core/engine.hpp"
#include "eunet/net/http_scenario.hpp"
#include "eunet/util/error.hpp"

namespace ui
{
    using namespace ftxui;

    class TuiApp
    {
    private:
        core::Orchestrator &orch_;
        core::NetworkEngine &engine_;

        ScreenInteractive screen_ = ScreenInteractive::Fullscreen();

        // ---------------- data ----------------
        std::mutex data_mtx_;
        std::vector<core::EventSnapshot> snapshots_;
        std::vector<std::string> menu_entries_;

        // ---------------- state ----------------
        int selected_menu_idx_ = 0;
        std::string current_url_ = "http://www.example.com";

        // ---------------- components ----------------
        Component input_url_;
        Component btn_go_;
        Component event_menu_;
        Component root_;

    public:
        TuiApp(core::Orchestrator &orch, core::NetworkEngine &engine)
            : orch_(orch), engine_(engine)
        {
            init_components();
            reset_session(); // ★ 启动即有内容
        }

        void run()
        {
            auto sink = std::make_shared<TuiSink>(
                screen_,
                [this](const core::EventSnapshot &snap)
                {
                    on_new_event(snap);
                });

            orch_.attach(sink);
            screen_.Loop(root_);
            orch_.detach(sink);
        }

    private:
        // ============================================================
        // init
        // ============================================================

        void init_components()
        {
            input_url_ = Input(&current_url_, "Enter URL");

            btn_go_ = Button(" GO ", [this]
                             {
                                 if (engine_.is_running())
                                     return;

                                 reset_session();
                                 engine_.execute(
                                     std::make_unique<net::http::HttpGetScenario>(
                                         current_url_)); });

            event_menu_ = Menu(
                &menu_entries_,
                &selected_menu_idx_,
                menu_option());

            // ---- focus tree (ONLY interactive components) ----
            auto input_row = Container::Horizontal({
                input_url_,
                btn_go_,
            });

            auto main_container = Container::Vertical({
                input_row,
                event_menu_,
            });

            // ---- root renderer ----
            root_ = Renderer(main_container, [&]
                             {
                                 std::lock_guard lock(data_mtx_);
                                 return render_layout(); });
        }

        // ============================================================
        // render
        // ============================================================

        Element render_layout()
        {
            return vbox({
                       render_header(),
                       render_input_bar(),
                       separator(),
                       render_content(),
                   }) |
                   border;
        }

        Element render_header()
        {
            bool running = engine_.is_running();
            return hbox({
                text(" EuNet Visualizer ") | bold | color(Color::Cyan),
                filler(),
                text(running ? " RUNNING " : " IDLE ") | bgcolor(running ? Color::Green : Color::GrayDark) | color(Color::Black),
            });
        }

        Element render_input_bar()
        {
            return hbox({
                       text(" URL: "),
                       input_url_->Render(),
                       btn_go_->Render(),
                   }) |
                   border;
        }

        Element render_content()
        {
            return hbox({
                event_menu_->Render() | frame | vscroll_indicator | flex,
                separator(),
                render_detail_panel() | size(WIDTH, EQUAL, 60),
            });
        }

        Element render_detail_panel()
        {
            if (snapshots_.empty() ||
                selected_menu_idx_ < 0 ||
                selected_menu_idx_ >= (int)snapshots_.size())
            {
                return vbox({
                           text("No Event Selected") | center,
                           separator(),
                           text("Press GO to start") | dim | center,
                       }) |
                       center;
            }

            const auto &snap = snapshots_[selected_menu_idx_];
            Color c = snapshot_color(snap);

            Elements lines;
            lines.push_back(text("DETAIL") | bold | color(c));
            lines.push_back(separator());
            lines.push_back(text("State:  " + to_string(snap.state)));
            lines.push_back(text("FD:     " + std::to_string(snap.fd)));
            lines.push_back(separator());
            lines.push_back(text("Message:") | bold);
            lines.push_back(paragraph(snap.event.msg));

            if (snap.error)
            {
                lines.push_back(separator());
                lines.push_back(text("ERROR") | color(Color::Red) | bold);
                lines.push_back(text(snap.error.format()) | color(Color::RedLight));
            }

            return vbox(std::move(lines));
        }

        // ============================================================
        // menu option
        // ============================================================

        MenuOption menu_option()
        {
            MenuOption opt;
            opt.entries_option.transform =
                [this](const EntryState &state)
            {
                if (state.index < 0 ||
                    state.index >= (int)snapshots_.size())
                    return text(state.label);

                const auto &snap = snapshots_[state.index];
                Color c = snapshot_color(snap);

                Element e = text(state.label);
                if (state.focused)
                    e = e | inverted | bold;
                else
                    e = e | color(c);

                return e;
            };
            return opt;
        }

        // ============================================================
        // data update
        // ============================================================

        void reset_session()
        {
            std::lock_guard lock(data_mtx_);

            snapshots_.clear();
            menu_entries_.clear();
            selected_menu_idx_ = 0;

            snapshots_.push_back(
                core::EventSnapshot{
                    core::Event::info(
                        core::EventType::CONNECTION_IDLE,
                        "Session started"),
                });

            snapshots_.back().state = core::LifeState::Init;
            menu_entries_.push_back("[i] Init        Session started");

            screen_.PostEvent(Event::Custom);
        }

        void on_new_event(const core::EventSnapshot &snap)
        {
            std::lock_guard lock(data_mtx_);

            snapshots_.push_back(snap);

            std::string preview = snap.event.msg;
            if (preview.size() > 40)
                preview = preview.substr(0, 37) + "...";

            menu_entries_.push_back(
                fmt::format("{} {:<10} {}",
                            snapshot_icon(snap),
                            to_string(snap.state),
                            preview));

            selected_menu_idx_ = menu_entries_.size() - 1;
            screen_.PostEvent(Event::Custom);
        }

        // ============================================================
        // helpers
        // ============================================================

        Color snapshot_color(const core::EventSnapshot &snap) const
        {
            if (!snap.error)
                return Color::Green;
            return Color::Red;
        }

        std::string snapshot_icon(const core::EventSnapshot &snap) const
        {
            return snap.error ? "[!]" : "[✔]";
        }
    };
}

#endif
