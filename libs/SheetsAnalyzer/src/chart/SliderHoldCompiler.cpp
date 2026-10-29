#include "SliderHoldCompiler.hpp"

#include "HoldJudgement.hpp"
#include "SheetsAnalyzer/SliderHoldPath.hpp"

namespace xlair::sheets::chart {
    namespace {
        constexpr s3d::int32 SliderLaneCount = 16;
    }

    Result<SliderHold> detail::CompileSliderHold(
        const s3d::Array<SliderHoldPoint>& points,
        const s3d::int64 sample_rate,
        const s3d::Array<TempoChange>& tempo_changes
    ) {
        if (points.size() < 2 || points.front().kind != SliderHoldPointKind::Start ||
            points.back().kind != SliderHoldPointKind::End) {
            return Result<SliderHold>::makeError(U"A slider hold needs a Start and an End.");
        }
        for (std::size_t index = 0; index < points.size(); ++index) {
            const auto& point = points[index];
            if (point.lane.width == 0 || point.lane.start >= SliderLaneCount ||
                (static_cast<s3d::int32>(point.lane.start) + point.lane.width) > SliderLaneCount ||
                (index > 0 && point.sample < points[index - 1].sample)) {
                return Result<SliderHold>::makeError(U"Invalid slider hold lane or point order.");
            }
            if (index > 0 && index + 1 < points.size() &&
                (point.kind == SliderHoldPointKind::Start || point.kind == SliderHoldPointKind::End)) {
                return Result<SliderHold>::makeError(U"Slider hold endpoints must be first and last.");
            }
        }

        // Path density is a rendering detail. Only musical intervals and visible
        // checkpoints contribute judgements, so normalized controls add no combo.
        s3d::Array<s3d::int64> visible_samples;
        for (const auto& point : points) {
            if (point.kind == SliderHoldPointKind::Visible) {
                visible_samples.push_back(point.sample);
            }
        }
        auto judge_samples = GenerateHoldJudgeSamples(
            points.front().sample,
            points.back().sample,
            sample_rate,
            tempo_changes,
            visible_samples
        );
        if (!judge_samples) {
            Result<SliderHold> result;
            result.diagnostics = std::move(judge_samples.diagnostics);
            return result;
        }
        SliderHold hold{ .points = points };
        hold.judge_points.reserve(judge_samples->size());
        for (const auto sample : *judge_samples) {
            hold.judge_points.push_back({ .sample = sample, .lane = *EvaluateSliderHoldLane(points, sample) });
        }
        return Result<SliderHold>{ std::move(hold) };
    }
}
