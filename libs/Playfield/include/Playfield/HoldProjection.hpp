#pragma once

#include <SheetsAnalyzer/Chart.hpp>

namespace xlair::playfield {
    struct PreparedHoldPoint {
        s3d::int64 sample = 0;
        sheets::TimelineIndex timeline = 0;
        long double position = 0.0L;
        double lane_start = 0.0;
        double lane_width = 1.0;
        double color_progress = 0.0;
    };

    struct PreparedHoldSpan {
        PreparedHoldPoint before;
        PreparedHoldPoint after;
        s3d::Array<PreparedHoldPoint> breakpoints;
        s3d::int64 first_sample = 0;
        double duration = 0.0;
    };

    struct HoldSection {
        PreparedHoldPoint before;
        PreparedHoldPoint after;
        long double before_distance = 0.0L;
        long double after_distance = 0.0L;
    };
}
