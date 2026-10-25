#include "LatencyCalibration.hpp"

#include <cmath>
#include <limits>

namespace xlair::app::flows {
    void LatencyCalibration::reset() {
        m_taps.clear();
        m_last_beat = -1;
        m_tap_count = 0;
        m_total_ms = 0.0;
    }

    bool LatencyCalibration::tap(const double time_ms) {
        if (!std::isfinite(time_ms) || time_ms < 0.0) {
            return false;
        }
        const double nearest = Max(0.0, std::floor((time_ms - FirstBeatMs) / BeatIntervalMs + 0.5));
        if (nearest >= static_cast<double>(std::numeric_limits<int64>::max())) {
            return false;
        }
        const auto beat = static_cast<int64>(nearest);
        const double offset = time_ms - (FirstBeatMs + nearest * BeatIntervalMs);
        const bool accepted = std::abs(offset) <= TapWindowMs && beat > m_last_beat;
        m_taps.push_back({ .time_ms = time_ms, .offset_ms = offset, .accepted = accepted });
        if (m_taps.size() > 16) {
            m_taps.pop_front();
        }
        if (accepted) {
            m_last_beat = beat;
            ++m_tap_count;
            m_total_ms += offset;
        }
        return accepted;
    }

    uint32 LatencyCalibration::tapCount() const noexcept {
        return m_tap_count;
    }

    Optional<double> LatencyCalibration::averageMs() const noexcept {
        return m_tap_count ? Optional<double>{ m_total_ms / m_tap_count } : none;
    }

    bool LatencyCalibration::ready() const noexcept {
        return m_tap_count >= MinimumTaps;
    }

    const Array<LatencyCalibration::Tap>& LatencyCalibration::taps() const noexcept {
        return m_taps;
    }
}
