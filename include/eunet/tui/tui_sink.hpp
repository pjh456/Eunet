#ifndef INCLUDE_EUNET_TUI_TUI_SINK
#define INCLUDE_EUNET_TUI_TUI_SINK

#include "ftxui/component/screen_interactive.hpp"

#include "eunet/core/sink.hpp"

namespace ui
{
    class TuiSink : public core::sink::IEventSink
    {
    private:
        ftxui::ScreenInteractive &screen_;
        std::function<void(const core::EventSnapshot &)> on_update_;

    public:
        TuiSink(ftxui::ScreenInteractive &screen,
                std::function<void(const core::EventSnapshot &)> callback)
            : screen_(screen), on_update_(callback) {}

        void on_event(const core::EventSnapshot &snap) override
        {
            // 1. 更新 TuiApp 内部的本地缓存数据
            on_update_(snap);

            // 2. 触发 FTXUI 的重绘事件（这是线程安全的）
            screen_.PostEvent(ftxui::Event::Custom);
        }
    };
}

#endif // INCLUDE_EUNET_TUI_TUI_SINK