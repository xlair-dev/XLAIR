#pragma once

#include "Common.hpp"

namespace xlair::app::flows {
    // The caller supplies time from audio playback, not a frame/wall clock.
    class LatencyCalibration {
    public:
        static constexpr double FirstBeatMs = 1000.0;
        static constexpr double BeatIntervalMs = 1000.0;
        static constexpr double TapWindowMs = 250.0;
        static constexpr uint32 MinimumTaps = 8;

        struct Tap {
            double time_ms = 0.0;
            double offset_ms = 0.0;
            bool accepted = false;
        };

        void reset();
        bool tap(double time_ms);

        [[nodiscard]]
        uint32 tapCount() const noexcept;

        [[nodiscard]]
        Optional<double> averageMs() const noexcept;

        [[nodiscard]]
        bool ready() const noexcept;

        [[nodiscard]]
        const Array<Tap>& taps() const noexcept;

    private:
        Array<Tap> m_taps;
        int64 m_last_beat = -1;
        uint32 m_tap_count = 0;
        double m_total_ms = 0.0;
    };
}
