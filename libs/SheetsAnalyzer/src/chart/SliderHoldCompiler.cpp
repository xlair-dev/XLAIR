#include "SliderHoldCompiler.hpp"

#include "HoldJudgement.hpp"

#include <algorithm>
#include <cmath>

namespace xlair::sheets::chart {
    namespace {
        constexpr long double LaneEdgeEpsilon = 1e-9L;
        constexpr s3d::int32 SliderLaneCount = 16;

        LaneSpan LaneAtSample(const s3d::Array<SliderHoldPoint>& points, const s3d::int64 sample) {
            const auto next = std::upper_bound(
                points.begin(),
                points.end(),
                sample,
                [](const s3d::int64 value, const SliderHoldPoint& point) {
                    return value < point.sample;
                }
            );
            if (next == points.begin()) {
                return points.front().lane;
            }
            if (next == points.end()) {
                return points.back().lane;
            }

            const auto& before = *std::prev(next);
            const auto& after = *next;
            const long double t = (static_cast<long double>(sample) - before.sample) /
                                  (static_cast<long double>(after.sample) - before.sample);
            const long double left_edge =
                s3d::Math::Lerp(static_cast<long double>(before.lane.start), static_cast<long double>(after.lane.start), t);
            const long double right_edge = s3d::Math::Lerp(
                static_cast<long double>(before.lane.start + before.lane.width),
                static_cast<long double>(after.lane.start + after.lane.width),
                t
            );
            const auto left =
                s3d::Clamp(static_cast<s3d::int32>(std::floor(left_edge + LaneEdgeEpsilon)), 0, SliderLaneCount - 1);
            const auto right =
                s3d::Clamp(static_cast<s3d::int32>(std::ceil(right_edge - LaneEdgeEpsilon)), left + 1, SliderLaneCount);
            return LaneSpan{ .start = static_cast<s3d::uint8>(left), .width = static_cast<s3d::uint8>(right - left) };
        }
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
        auto judge_samples =
            GenerateHoldJudgeSamples(points.front().sample, points.back().sample, sample_rate, tempo_changes);
        if (!judge_samples) {
            Result<SliderHold> result;
            result.diagnostics = std::move(judge_samples.diagnostics);
            return result;
        }
        for (const auto& point : points) {
            if (point.kind == SliderHoldPointKind::Visible) {
                judge_samples->push_back(point.sample);
            }
        }
        judge_samples->sort();
        judge_samples->erase(std::unique(judge_samples->begin(), judge_samples->end()), judge_samples->end());

        SliderHold hold{ .points = points };
        hold.judge_points.reserve(judge_samples->size());
        for (const auto sample : *judge_samples) {
            hold.judge_points.push_back({ .sample = sample, .lane = LaneAtSample(points, sample) });
        }
        return Result<SliderHold>{ std::move(hold) };
    }
}
