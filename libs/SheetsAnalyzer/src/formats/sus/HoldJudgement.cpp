#include "HoldJudgement.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace xlair::sheets::formats::sus {
    namespace {
        constexpr std::size_t MaximumJudgePointCount = 1'000'000;

        [[nodiscard]]
        s3d::int64 RoundSample(const long double sample) {
            constexpr auto Minimum = static_cast<long double>(std::numeric_limits<s3d::int64>::min());
            constexpr auto Maximum = static_cast<long double>(std::numeric_limits<s3d::int64>::max());
            return static_cast<s3d::int64>(s3d::Clamp(std::round(sample), Minimum, Maximum));
        }

        [[nodiscard]]
        double BPMAtSample(const s3d::Array<TempoChange>& tempo_changes, const long double sample) {
            const auto change = std::upper_bound(
                tempo_changes.begin(),
                tempo_changes.end(),
                sample,
                [](const long double value, const TempoChange& candidate) {
                    return value < candidate.sample;
                }
            );
            return (change == tempo_changes.begin() ? change : std::prev(change))->bpm;
        }

        [[nodiscard]]
        s3d::int64 NextJudgeSample(
            const s3d::int64 current_sample, const s3d::int64 sample_rate, const s3d::Array<TempoChange>& tempo_changes
        ) {
            long double cursor = current_sample;
            const double current_bpm = BPMAtSample(tempo_changes, cursor);
            const long double division = current_bpm < 120.0 ? 4.0L : (current_bpm < 240.0 ? 2.0L : 1.0L);
            long double remaining_beats = 1.0L / division;

            while (remaining_beats > 0.0L) {
                const double bpm = BPMAtSample(tempo_changes, cursor);
                const long double samples_per_beat =
                    static_cast<long double>(sample_rate) * 60.0L / static_cast<long double>(bpm);
                const auto next_change = std::upper_bound(
                    tempo_changes.begin(),
                    tempo_changes.end(),
                    cursor,
                    [](const long double value, const TempoChange& candidate) {
                        return value < candidate.sample;
                    }
                );

                if (next_change == tempo_changes.end()) {
                    return RoundSample(cursor + (remaining_beats * samples_per_beat));
                }

                const long double beats_until_change =
                    (static_cast<long double>(next_change->sample) - cursor) / samples_per_beat;
                if (remaining_beats <= beats_until_change) {
                    return RoundSample(cursor + (remaining_beats * samples_per_beat));
                }

                remaining_beats -= beats_until_change;
                cursor = next_change->sample;
            }

            return RoundSample(cursor);
        }
    }

    Result<s3d::Array<s3d::int64>> detail::GenerateHoldJudgeSamples(
        const s3d::int64 start_sample,
        const s3d::int64 end_sample,
        const s3d::int64 sample_rate,
        const s3d::Array<TempoChange>& tempo_changes
    ) {
        if (end_sample <= start_sample) {
            return Result<s3d::Array<s3d::int64>>::makeError(U"A Hold End point must be placed after its Start point.");
        }
        if (tempo_changes.isEmpty()) {
            return Result<s3d::Array<s3d::int64>>::makeError(
                U"Hold judgement generation requires at least one tempo definition."
            );
        }

        s3d::Array<s3d::int64> samples = { start_sample };
        s3d::int64 current_sample = start_sample;
        while (true) {
            const s3d::int64 next_sample = NextJudgeSample(current_sample, sample_rate, tempo_changes);
            if (next_sample <= current_sample) {
                return Result<s3d::Array<s3d::int64>>::makeError(
                    U"A Hold judgement interval is shorter than one audio sample."
                );
            }
            if (next_sample >= end_sample) {
                break;
            }

            samples.push_back(next_sample);
            if (samples.size() > MaximumJudgePointCount) {
                return Result<s3d::Array<s3d::int64>>::makeError(U"A Hold requires too many judgement points.");
            }
            current_sample = next_sample;
        }
        samples.push_back(end_sample);
        return Result<s3d::Array<s3d::int64>>{ std::move(samples) };
    }
}
