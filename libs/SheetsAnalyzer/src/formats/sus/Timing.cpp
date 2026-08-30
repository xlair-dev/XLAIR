#include "Timing.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace xlair::sheets::formats::sus {
    namespace {
        constexpr double DefaultBPM = 120.0;
        constexpr long double DefaultBeatsPerMeasure = 4.0L;

        [[nodiscard]]
        s3d::int64 RoundSample(const long double sample) {
            constexpr auto Minimum = static_cast<long double>(std::numeric_limits<s3d::int64>::min());
            constexpr auto Maximum = static_cast<long double>(std::numeric_limits<s3d::int64>::max());
            return static_cast<s3d::int64>(s3d::Clamp(std::round(sample), Minimum, Maximum));
        }
    }

    TimingMap::TimingMap(
        const s3d::int64 sample_rate,
        const s3d::uint32 ticks_per_beat,
        const long double offset_samples
    )
        : m_sample_rate{ sample_rate }, m_ticks_per_beat{ ticks_per_beat } {
        m_measure_segments.push_back(
            {
                .measure = 0,
                .beat = 0.0L,
                .beats_per_measure = DefaultBeatsPerMeasure,
            }
        );
        m_segments.push_back(
            {
                .beat = 0.0L,
                .sample = offset_samples,
                .bpm = DefaultBPM,
            }
        );
    }

    Result<TimingMap> TimingMap::Build(const Document& document, const TimingOptions& options) {
        if (options.sample_rate <= 0) {
            return Result<TimingMap>::makeError(U"The sample rate must be a positive integer.");
        }
        if (document.ticks_per_beat == 0) {
            return Result<TimingMap>::makeError(U"Ticks per beat must be a positive integer.");
        }
        if (!std::isfinite(options.offset_seconds)) {
            return Result<TimingMap>::makeError(U"The chart offset must be finite.");
        }

        const long double offset_samples =
            static_cast<long double>(options.offset_seconds) * static_cast<long double>(options.sample_rate);
        if (!std::isfinite(offset_samples)) {
            return Result<TimingMap>::makeError(U"The chart offset exceeds the supported timing range.");
        }

        TimingMap timing{ options.sample_rate, document.ticks_per_beat, offset_samples };

        s3d::Array<std::pair<s3d::uint32, double>> measure_changes;
        measure_changes.reserve(document.beats_per_measure.size());
        for (const auto& [measure, beats] : document.beats_per_measure) {
            if (!std::isfinite(beats) || beats <= 0.0) {
                return Result<TimingMap>::makeError(U"Measure lengths must be positive finite numbers.");
            }
            measure_changes.push_back({ measure, beats });
        }
        measure_changes.sort_by([](const auto& left, const auto& right) {
            return left.first < right.first;
        });

        for (const auto& [measure, beats] : measure_changes) {
            const auto& previous = timing.m_measure_segments.back();
            const long double beat =
                previous.beat + (static_cast<long double>(measure - previous.measure) * previous.beats_per_measure);
            if (!std::isfinite(beat)) {
                return Result<TimingMap>::makeError(U"Measure positions exceed the supported timing range.");
            }

            if (measure == previous.measure) {
                timing.m_measure_segments.back().beats_per_measure = beats;
            } else {
                timing.m_measure_segments.push_back(
                    {
                        .measure = measure,
                        .beat = beat,
                        .beats_per_measure = beats,
                    }
                );
            }
        }

        struct BPMEvent {
            long double beat = 0.0L;
            double bpm = DefaultBPM;
        };

        s3d::Array<BPMEvent> bpm_events;
        bpm_events.reserve(document.bpm_changes.size());
        for (const auto& change : document.bpm_changes) {
            if (change.position.denominator == 0 || change.position.numerator >= change.position.denominator) {
                return Result<TimingMap>::makeError(
                    U"Timing positions require a non-zero denominator and a numerator smaller than it."
                );
            }

            const auto definition = document.bpm_definitions.find(change.definition);
            if (definition == document.bpm_definitions.end()) {
                return Result<TimingMap>::makeError(U"BPM changes must reference a defined BPM value.");
            }
            if (!std::isfinite(definition->second) || definition->second <= 0.0) {
                return Result<TimingMap>::makeError(U"BPM values must be positive finite numbers.");
            }

            bpm_events.push_back(
                {
                    .beat = timing.absoluteBeat(change.position),
                    .bpm = definition->second,
                }
            );
        }
        bpm_events.stable_sort_by([](const auto& left, const auto& right) {
            return left.beat < right.beat;
        });

        for (const auto& event : bpm_events) {
            const auto& previous = timing.m_segments.back();
            const long double samples_per_beat = static_cast<long double>(timing.m_sample_rate) * 60.0L / previous.bpm;
            const long double sample = previous.sample + ((event.beat - previous.beat) * samples_per_beat);
            if (!std::isfinite(sample)) {
                return Result<TimingMap>::makeError(U"BPM changes exceed the supported timing range.");
            }

            timing.m_segments.push_back(
                {
                    .beat = event.beat,
                    .sample = sample,
                    .bpm = event.bpm,
                }
            );
        }

        return Result<TimingMap>{ std::move(timing) };
    }

    s3d::int64 TimingMap::toSample(const Position& position) const {
        return sampleAt(absoluteBeat(position));
    }

    s3d::int64 TimingMap::toSample(const TickPosition& position) const {
        return sampleAt(absoluteBeat(position));
    }

    double TimingMap::bpmAt(const Position& position) const {
        return segmentAt(absoluteBeat(position)).bpm;
    }

    const TimingMap::MeasureSegment& TimingMap::measureSegmentAt(const s3d::uint32 measure) const {
        // upper_bound points just after the requested measure; the preceding entry is the active length segment.
        const auto segment = std::upper_bound(
            m_measure_segments.begin(),
            m_measure_segments.end(),
            measure,
            [](const s3d::uint32 value, const MeasureSegment& candidate) {
                return value < candidate.measure;
            }
        );
        return *(segment == m_measure_segments.begin() ? segment : std::prev(segment));
    }

    const TimingMap::Segment& TimingMap::segmentAt(const long double beat) const {
        // Multiple BPM changes may share a beat. Stepping back from upper_bound selects the last applied change.
        const auto segment = std::upper_bound(
            m_segments.begin(),
            m_segments.end(),
            beat,
            [](const long double value, const Segment& candidate) {
                return value < candidate.beat;
            }
        );
        return *(segment == m_segments.begin() ? segment : std::prev(segment));
    }

    s3d::int64 TimingMap::sampleAt(const long double beat) const {
        const auto& active = segmentAt(beat);
        const long double samples_per_beat = static_cast<long double>(m_sample_rate) * 60.0L / active.bpm;
        return RoundSample(active.sample + ((beat - active.beat) * samples_per_beat));
    }

    long double TimingMap::absoluteBeat(const Position& position) const {
        // Convert the measure-relative fraction into one monotonically increasing beat coordinate.
        const auto& active = measureSegmentAt(position.measure);
        const long double measure_beat =
            active.beat + (static_cast<long double>(position.measure - active.measure) * active.beats_per_measure);
        const long double denominator = position.denominator == 0 ? 1.0L : position.denominator;
        return measure_beat + (active.beats_per_measure * static_cast<long double>(position.numerator) / denominator);
    }

    long double TimingMap::absoluteBeat(const TickPosition& position) const {
        // #TIL ticks are beat-based and may extend beyond the nominal end of their source measure.
        const auto& active = measureSegmentAt(position.measure);
        const long double measure_beat =
            active.beat + (static_cast<long double>(position.measure - active.measure) * active.beats_per_measure);
        return measure_beat + (static_cast<long double>(position.tick) / m_ticks_per_beat);
    }
}
