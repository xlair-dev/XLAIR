#include "Timing.hpp"
#include "SampleMath.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace xlair::sheets::chart {
    namespace {
        long double Beat(const CcPosition position) {
            return static_cast<long double>(position.numerator) / position.denominator;
        }
    }

    TimingMap::TimingMap(s3d::Array<Segment> segments) : m_segments{ std::move(segments) } {}

    Result<TimingMap> TimingMap::Build(const std::span<const CcTempo> tempos, const ChartLoadOptions& options) {
        if (options.sample_rate <= 0 || !std::isfinite(options.offset_seconds)) {
            return Result<TimingMap>::makeError(U"Timing requires a positive sample rate and a finite offset.");
        }
        const long double rate = options.sample_rate;
        const long double offset = static_cast<long double>(options.offset_seconds) * rate;
        if (!std::isfinite(offset)) {
            return Result<TimingMap>::makeError(U"The chart offset exceeds the supported timing range.");
        }
        s3d::Array<Segment> segments{ { 0.0L, offset, rate * 0.5L } };
        for (const auto& tempo : tempos) {
            if (tempo.position.denominator == 0 || !std::isfinite(tempo.bpm) || tempo.bpm <= 0.0) {
                return Result<TimingMap>::makeError(U"Invalid tempo in ChartConverter IR.");
            }
            const long double beat = Beat(tempo.position);
            const auto& previous = segments.back();
            const long double sample = previous.sample + (beat - previous.beat) * previous.samples_per_beat;
            const long double samples_per_beat = rate * 60.0L / tempo.bpm;
            if (beat < previous.beat || !std::isfinite(sample) || !std::isfinite(samples_per_beat)) {
                return Result<TimingMap>::makeError(U"Tempo events exceed the supported timing range or are unsorted.");
            }
            segments.push_back({ beat, sample, samples_per_beat });
        }
        return Result<TimingMap>{ TimingMap{ std::move(segments) } };
    }

    s3d::int64 TimingMap::toSample(const CcPosition position) const {
        return sampleAtBeat(Beat(position));
    }

    s3d::int64 TimingMap::sampleAtBeat(const long double beat) const {
        // Events and query positions already use absolute beats, independent of source format.
        const auto next =
            std::upper_bound(m_segments.begin(), m_segments.end(), beat, [](long double value, const Segment& segment) {
                return value < segment.beat;
            });
        const auto& active = *std::prev(next);
        return detail::RoundSample(active.sample + (beat - active.beat) * active.samples_per_beat);
    }
}
