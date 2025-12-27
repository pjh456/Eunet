#ifndef INCLUDE_EUNET_CORE_SINK_METRICS_SINK
#define INCLUDE_EUNET_CORE_SINK_METRICS_SINK

#include <atomic>

#include "eunet/core/event_snap_shoot.hpp"
#include "eunet/core/sink/sink.hpp"

namespace core::sink
{
    struct Metrics
    {
        std::atomic<size_t> total_events{0};
        std::atomic<size_t> errors{0};
    };

    class MetricsSink : public IEventSink
    {
        Metrics m;

    public:
        void on_event(const EventSnapshot &s) override
        {
            ++m.total_events;
            if (s.has_error)
                ++m.errors;
        }

        const Metrics &snapshot() const noexcept { return m; }
    };

}

#endif // INCLUDE_EUNET_CORE_SINK_METRICS_SINK