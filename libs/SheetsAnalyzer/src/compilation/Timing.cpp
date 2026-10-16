#include "Timing.hpp"
#include "SampleMath.hpp"

#include <algorithm>
#include <cmath>
#include <span>

namespace xlair::sheets::compilation {
    Result<TimingMap> TimingMap::Build(const CcChartView& source, const ChartLoadOptions& options) {
        if (options.sample_rate <= 0 || !std::isfinite(options.offset_seconds)) {
            return Result<TimingMap>::makeError(U"The sample rate must be positive and the chart offset finite.");
        }
        TimingMap timing;
        timing.m_sample_rate = options.sample_rate;
        const long double offset_samples = static_cast<long double>(options.offset_seconds) * options.sample_rate;
        if (!std::isfinite(offset_samples)) {
            return Result<TimingMap>::makeError(U"The chart offset exceeds the supported timing range.");
        }
        timing.m_segments.push_back({ .sample = offset_samples });
        for (const auto& tempo : std::span{ source.tempos, source.tempo_count }) {
            const long double beat = static_cast<long double>(tempo.position.numerator) / tempo.position.denominator;
            const auto& previous = timing.m_segments.back();
            const long double sample =
                previous.sample + (beat - previous.beat) * options.sample_rate * 60.0L / previous.bpm;
            if (!std::isfinite(sample)) {
                return Result<TimingMap>::makeError(U"Tempo changes exceed the supported timing range.");
            }
            timing.m_segments.push_back({ .beat = beat, .sample = sample, .bpm = tempo.bpm });
        }
        return Result<TimingMap>{ std::move(timing) };
    }

    s3d::int64 TimingMap::toSample(const CcPosition position) const {
        const long double beat = static_cast<long double>(position.numerator) / position.denominator;
        const auto next = std::upper_bound(
            m_segments.begin(),
            m_segments.end(),
            beat,
            [](const long double value, const Segment& segment) {
                return value < segment.beat;
            }
        );
        const auto& active = *(next == m_segments.begin() ? next : std::prev(next));
        return detail::RoundSample(active.sample + (beat - active.beat) * m_sample_rate * 60.0L / active.bpm);
    }

    s3d::Array<TempoChange> TimingMap::tempoChanges() const {
        s3d::Array<TempoChange> changes;
        for (const auto& segment : m_segments) {
            changes.push_back({ .sample = detail::RoundSample(segment.sample), .bpm = segment.bpm });
        }
        return changes;
    }
}
