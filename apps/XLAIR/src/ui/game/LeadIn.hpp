#pragma once

#include <SheetsAnalyzer/Chart.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace xlair::ui::game {
    class LeadIn {
    public:
        explicit LeadIn(
            const sheets::Chart& chart,
            const s3d::int64 judgment_offset_samples,
            const double duration_seconds = 0.8
        )
            : m_sample_rate{ static_cast<long double>(chart.sample_rate) } {
            if (chart.sample_rate <= 0 || !std::isfinite(duration_seconds) || duration_seconds <= 0.0) {
                throw std::invalid_argument{
                    "A lead-in requires a positive sample rate and finite positive duration."
                };
            }

            s3d::int64 first_sample = 0;
            for (const auto& note : chart.slider_notes) {
                first_sample = std::min(first_sample, note.sample);
            }
            for (const auto& note : chart.side_notes) {
                first_sample = std::min(first_sample, note.sample);
            }
            for (const auto& hold : chart.slider_holds) {
                if (!hold.judge_points.isEmpty()) {
                    first_sample = std::min(first_sample, hold.judge_points.front().sample);
                }
            }
            for (const auto& hold : chart.side_holds) {
                if (!hold.judge_samples.isEmpty()) {
                    first_sample = std::min(first_sample, hold.judge_samples.front());
                }
            }

            // Leave the early hit window visible even when input compensation moves judged time forward.
            const long double duration_samples =
                std::max(1.0L, std::round(m_sample_rate * static_cast<long double>(duration_seconds)));
            const long double start = static_cast<long double>(first_sample) +
                                      std::min(0.0L, static_cast<long double>(judgment_offset_samples)) -
                                      duration_samples;
            m_position = std::max(MinimumSample, start);
        }

        void update(const double delta_seconds) {
            if (!std::isfinite(delta_seconds) || delta_seconds <= 0.0 || finished()) {
                return;
            }
            const long double advance = static_cast<long double>(delta_seconds) * m_sample_rate;
            m_position = advance >= -m_position ? 0.0L : m_position + advance;
        }

        [[nodiscard]]
        s3d::int64 sample() const {
            if (m_position <= MinimumSample) {
                return std::numeric_limits<s3d::int64>::min();
            }
            return static_cast<s3d::int64>(std::floor(m_position));
        }

        [[nodiscard]]
        bool finished() const {
            return m_position == 0.0L;
        }

    private:
        static constexpr long double MinimumSample = static_cast<long double>(std::numeric_limits<s3d::int64>::min());

        long double m_sample_rate;
        long double m_position = 0.0L;
    };
}
