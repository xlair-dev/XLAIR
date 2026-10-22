#pragma once

#include "Chart.hpp"

namespace xlair::sheets {
    // Expects validated lanes and points sorted by nondecreasing sample.
    // Linearly evaluates the path and rounds its edges outward to covered lanes.
    // Queries outside the path use its endpoint; coincident points use the last
    // point at that sample. Returns none only when the path is empty.
    [[nodiscard]]
    s3d::Optional<LaneSpan> EvaluateSliderHoldLane(const s3d::Array<SliderHoldPoint>& points, s3d::int64 sample);
}
