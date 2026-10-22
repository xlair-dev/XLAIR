#include "SheetsAnalyzer/SliderHoldPath.hpp"

#include <algorithm>
#include <cmath>

namespace xlair::sheets {
    namespace {
        constexpr long double LaneEdgeEpsilon = 1e-9L;
        constexpr s3d::int32 SliderLaneCount = 16;
    }

    s3d::Optional<LaneSpan> EvaluateSliderHoldLane(const s3d::Array<SliderHoldPoint>& points, const s3d::int64 sample) {
        if (points.isEmpty()) {
            return s3d::none;
        }
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
