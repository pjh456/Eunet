/*
 * ============================================================================
 *  File Name   : tui_app.hpp
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

    /**
     * @brief 终端用户界面应用程序
     *
     * 基于 FTXUI 库构建。负责渲染事件列表、详情面板和状态栏。
     * 该类运行在 UI 线程，通过 shared memory (pending_ events) 与网络线程通信。
     */
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
        TuiApp(
            core::Orchestrator &orch,
            core::NetworkEngine &engine);

        /**
         * @brief 启动 UI 主循环
         *
         * 注册 Sink 到 Orchestrator，并阻塞当前线程进行界面渲染。
         * 直到用户退出或收到终止信号。
         */
        void run();

    private:
        // ============================================================
        // init
        // ============================================================

        void init_components();

        // ============================================================
        // render
        // ============================================================

        Element render_layout();

        Element render_header();

        Element render_content();

        Element render_detail_panel();

        // ============================================================
        // menu option
        // ============================================================

        MenuOption menu_option();

        // ============================================================
        // data update
        // ============================================================

        void reset_session();

        /**
         * @brief 网络事件回调
         *
         * 当 Orchestrator 分发新事件时调用。
         * 该函数在网络线程上下文中执行，必须加锁并将事件放入待处理队列，
         * 然后通过 screen_.PostEvent 通知 UI 线程刷新。
         */
        void on_new_event(const core::EventSnapshot &snap);

        /**
         * @brief 应用待处理事件
         *
         * 在 UI 渲染帧开始前调用。将 pending_ 队列中的事件移动到
         * 用于显示的 snapshots_ 列表中，并更新菜单选中项。
         */
        void apply_pending_events();

        // ============================================================
        // helpers
        // ============================================================

        Color snapshot_color(const core::EventSnapshot &snap) const;

        std::string snapshot_icon(const core::EventSnapshot &snap) const;

        std::string sanitize_for_tui(
            std::string_view s,
            size_t max_len = 512);
    };
}

#endif
