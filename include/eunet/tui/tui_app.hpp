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
        static constexpr size_t MAX_EVENTS = 2000;

        core::Orchestrator &orch_;
        core::NetworkEngine &engine_;

        ScreenInteractive screen_ = ScreenInteractive::Fullscreen();

        std::vector<core::EventSnapshot> pending_;

        // ---------------- data ----------------
        std::mutex data_mtx_;
        std::vector<core::EventSnapshot> snapshots_;
        std::vector<std::string> dummy_entries_; // 只占位

        // ---------------- state ----------------
        int selected_menu_idx_ = 0;
        std::string current_url_ = "http://www.example.com";

        // ---------------- components ----------------
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
            event_menu_ = Menu(
                &dummy_entries_,
                &selected_menu_idx_,
                menu_option());

            auto main_container = Container::Vertical({
                // input_row,
                event_menu_,
            });

            // ---- root renderer ----
            root_ = Renderer(
                main_container,
                [&]
                {
                    apply_pending_events();
                    return render_layout();
                });
        }

        // ============================================================
        // render
        // ============================================================

        Element render_layout()
        {
            return vbox({
                       render_header(),
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

        Element render_content()
        {
            return hbox({
                event_menu_->Render() | flex,
                separator(),
                render_detail_panel() | flex,
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
                           text("Waiting for events...") | dim | center,
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
                    return text("");

                const auto &snap = snapshots_[state.index];
                Color c = snapshot_color(snap);

                std::string preview = " ";
                preview += snap.event.msg;
                if (preview.size() > 40)
                    preview = preview.substr(0, 37) + "...";

                Element e = hbox({
                    text(snapshot_icon(snap)) | color(c),
                    text(to_string(snap.state)) | bold,
                    text(preview) | dim,
                });

                if (state.focused)
                    e = e | inverted;

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
            pending_.clear();
            dummy_entries_.clear();
            selected_menu_idx_ = 0;

            snapshots_.push_back(
                core::EventSnapshot{
                    core::Event::info(
                        core::EventType::CONNECTION_IDLE,
                        "Session started"),
                    0, core::LifeState::Init});

            dummy_entries_.resize(1);
        }

        void on_new_event(const core::EventSnapshot &snap)
        {
            {
                std::lock_guard lock(data_mtx_);
                pending_.push_back(snap);
            }
            screen_.PostEvent(Event::Custom);
        }

        void apply_pending_events()
        {
            std::lock_guard lock(data_mtx_);

            for (auto &snap : pending_)
            {
                snapshots_.push_back(snap);

                if (snapshots_.size() > MAX_EVENTS)
                {
                    snapshots_.erase(snapshots_.begin());
                    if (selected_menu_idx_ > 0)
                        --selected_menu_idx_;
                }
            }

            if (!pending_.empty())
                selected_menu_idx_ = snapshots_.size() - 1;

            dummy_entries_.resize(snapshots_.size());

            pending_.clear();
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
