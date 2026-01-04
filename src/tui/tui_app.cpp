/*
 * ============================================================================
 *  File Name   : tui_app.cpp
 *  Module      : tui
 *
 *  Description :
 *      TUI 应用程序的主入口类。基于 FTXUI 库构建终端界面，
 *      负责渲染事件列表、详情面板以及处理用户输入。
 *      维护 UI 层的本地状态快照。
 *
 *  Third-Party Dependencies :
 *      - FTXUI
 *          Usage     : 终端用户界面构建与渲染
 *          License   : MIT License
 *
 *      - fmt
 *          Usage     : 字符串格式化
 *          License   : MIT License
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#include "eunet/tui/tui_app.hpp"

#include "fmt/format.h"

#include <cctype>

namespace ui
{
    TuiApp::TuiApp(
        core::Orchestrator &orch,
        core::NetworkEngine &engine)
        : orch_(orch), engine_(engine)
    {
        init_components();
        reset_session();
    }

    void TuiApp::run()
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

    void TuiApp::init_components()
    {
        // 配置 Input
        InputOption opt;
        opt.content = &input_url_val_;
        opt.placeholder = "Enter URL (e.g., www.baidu.com)";

        auto input_base = Input(opt);

        input_component_ =
            input_base |
            CatchEvent(
                [this](Event event)
                {
                    // 拦截回车键
                    if (event == Event::Return)
                    {
                        trigger_scenario();
                        // 返回 true 表示事件已被处理，不再传递给 Input 组件，防止插入 \n
                        return true;
                    }
                    return false;
                }) |
            size(HEIGHT, EQUAL, 1); // 强制高度为 1

        event_menu_ = Menu(
            &dummy_entries_,
            &selected_menu_idx_,
            menu_option());

        auto container = Container::Vertical({
            input_component_,
            event_menu_,
        });

        // ---- root renderer ----
        root_ = Renderer(
            container,
            [this]
            {
                apply_pending_events();
                return vbox({
                           render_header(),
                           separator(),
                           // 渲染输入框
                           hbox({
                               text(" Target: "),
                               input_component_->Render() | flex,
                           }) | size(HEIGHT, EQUAL, 1), // 渲染时也限制高度
                           separator(),
                           // 渲染内容
                           render_content(),
                       }) |
                       border;
            });
    }

    Element TuiApp::render_layout()
    {
        return vbox({
                   render_header(),
                   separator(),
                   input_component_->Render() | border,
                   separator(),
                   render_content(),
               }) |
               border;
    }

    Element TuiApp::render_header()
    {
        bool running = engine_.is_running();
        return hbox({
            text(" EuNet Visualizer ") | bold | color(Color::Cyan),
            filler(),
            text(running ? " RUNNING " : " IDLE ") |
                bgcolor(running ? Color::Green : Color::GrayDark) |
                color(Color::Black),
        });
    }

    Element TuiApp::render_content()
    {
        return hbox({
            event_menu_->Render() | frame | vscroll_indicator | size(WIDTH, LESS_THAN, 40) | flex,
            separator(),
            render_detail_panel() | size(WIDTH, GREATER_THAN, 60) | flex,
        });
    }

    Element TuiApp::render_detail_panel()
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
        lines.push_back(paragraph(sanitize_for_tui(snap.event.msg)));

        if (snap.payload.has_value() && !snap.payload->empty())
        {
            lines.push_back(separator());
            lines.push_back(text("Payload (Hex Dump)") | bold);

            std::string hex_view = format_hex_dump(*snap.payload);
            lines.push_back(paragraph(hex_view));
        }

        if (snap.error)
        {
            lines.push_back(separator());
            lines.push_back(text("ERROR") | color(Color::Red) | bold);
            lines.push_back(text(snap.error->format()) | color(Color::RedLight));
        }

        return vbox(std::move(lines)) | frame | vscroll_indicator;
    }

    MenuOption TuiApp::menu_option()
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

    void TuiApp::reset_session()
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

    void TuiApp::on_new_event(
        const core::EventSnapshot &snap)
    {
        {
            std::lock_guard lock(data_mtx_);
            pending_.push_back(snap);
        }
        screen_.PostEvent(Event::Custom);
    }

    void TuiApp::apply_pending_events()
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

    void TuiApp::trigger_scenario()
    {
        if (engine_.is_running())
            return;

        // 清理 UI 数据
        reset_session();

        // 清理底层数据
        orch_.reset();

        // 使用清理后的 URL
        std::string safe_url = clean_url(input_url_val_);
        if (safe_url.empty())
            return;

        // 把清理后的 URL 写回输入框
        input_url_val_ = safe_url;

        // 执行新请求
        engine_.execute(
            std::make_unique<
                net::http::HttpGetScenario>(
                safe_url));
    }

    Color TuiApp::snapshot_color(
        const core::EventSnapshot &snap)
    {
        if (!snap.error)
            return Color::Green;
        return Color::Red;
    }

    std::string TuiApp::snapshot_icon(
        const core::EventSnapshot &snap)
    {
        return snap.error.has_value() ? "[!]" : "[✔]";
    }

    std::string
    TuiApp::sanitize_for_tui(
        std::string_view s,
        size_t max_len)
    {
        std::string out;
        out.reserve(std::min(s.size(), max_len));

        for (unsigned char c : s)
        {
            if (out.size() >= max_len)
                break;

            // 允许的字符：可打印 ASCII + 换行
            if (c == '\n' || c == '\r' || c == '\t')
                out.push_back(c);
            else if (c >= 0x20 && c < 0x7f)
                out.push_back(c);
            else
                out.push_back('.'); // 一律替换
        }

        if (s.size() > max_len)
            out += "...";

        return out;
    }

    std::string TuiApp::clean_url(std::string s)
    {
        s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
        s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
        // 去除前后空格
        s.erase(0, s.find_first_not_of(" \t"));
        s.erase(s.find_last_not_of(" \t") + 1);
        return s;
    }

    std::string TuiApp::format_hex_dump(
        std::span<const std::byte> data)
    {
        if (data.empty())
        {
            return "[ Empty Payload ]";
        }

        std::stringstream ss;
        constexpr size_t bytes_per_line = 16;

        for (size_t offset = 0; offset < data.size(); offset += bytes_per_line)
        {
            // 1. Offset
            ss << fmt::format("{:08x}: ", offset);

            // 2. Hex bytes
            for (size_t i = 0; i < bytes_per_line; ++i)
            {
                if (offset + i < data.size())
                {
                    ss << fmt::format("{:02x} ", static_cast<unsigned char>(data[offset + i]));
                }
                else
                {
                    ss << "   "; // padding
                }
                if (i == 7)
                {
                    ss << " "; // separator in the middle
                }
            }
            ss << " |";

            // 3. ASCII chars
            for (size_t i = 0; i < bytes_per_line && offset + i < data.size(); ++i)
            {
                char c = static_cast<char>(data[offset + i]);
                if (std::isprint(static_cast<unsigned char>(c)))
                {
                    ss << c;
                }
                else
                {
                    ss << '.';
                }
            }
            ss << "\n";
        }
        return ss.str();
    }
}