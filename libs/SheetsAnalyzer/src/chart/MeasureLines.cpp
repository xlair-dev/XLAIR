#include "MeasureLines.hpp"

#include <cmath>
#include <utility>

namespace xlair::sheets::chart {
    namespace {
        long double Beat(const CcPosition position) {
            return static_cast<long double>(position.numerator) / position.denominator;
        }
    }

    Result<s3d::Array<MeasureLine>> CompileMeasureLines(const CcResult& parsed, const TimingMap& timing) {
        const auto view = cc_result_chart(&parsed);
        const auto metadata = cc_result_metadata(&parsed);
        const std::span lengths{ metadata.measure_lengths, metadata.measure_length_count };
        for (std::size_t index = 0; index < lengths.size(); ++index) {
            if (lengths[index].length.denominator == 0 || lengths[index].length.numerator == 0 ||
                (index > 0 && lengths[index].measure <= lengths[index - 1].measure)) {
                return Result<s3d::Array<MeasureLine>>::makeError(U"Invalid measure lengths in ChartConverter IR.");
            }
        }

        long double last_beat = 0.0L;
        for (const auto& note : std::span{ view.notes, view.note_count }) {
            last_beat = s3d::Max(last_beat, Beat(note.position));
            for (const auto& point : std::span{ note.points, note.point_count }) {
                last_beat = s3d::Max(last_beat, Beat(point.position));
            }
        }

        s3d::Array<MeasureLine> lines;
        std::size_t length_index = 0;
        long double beats_per_measure = 4.0L;
        long double segment_beat = 0.0L;
        s3d::uint32 segment_measure = 0;
        // Bound work for maliciously sparse or excessively long charts.
        constexpr s3d::uint32 MaximumMeasureLines = 1'000'000;
        for (s3d::uint32 measure = 0; measure < MaximumMeasureLines; ++measure) {
            const long double beat = segment_beat + (measure - segment_measure) * beats_per_measure;
            if (!std::isfinite(beat)) {
                return Result<s3d::Array<MeasureLine>>::makeError(U"Measure positions exceed the supported range.");
            }
            if (length_index < lengths.size() && lengths[length_index].measure == measure) {
                segment_beat = beat;
                segment_measure = measure;
                beats_per_measure = Beat(lengths[length_index++].length);
            }
            lines.push_back({ .measure = measure, .sample = timing.sampleAtBeat(beat) });
            // Include the first boundary after the final note or hold tail.
            if (beat > last_beat) {
                return Result<s3d::Array<MeasureLine>>{ std::move(lines) };
            }
        }
        return Result<s3d::Array<MeasureLine>>::makeError(U"The chart requires too many measure lines.");
    }
}
