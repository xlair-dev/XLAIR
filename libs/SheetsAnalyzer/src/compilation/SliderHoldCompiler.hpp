#pragma once

#include "SheetsAnalyzer/Chart.hpp"
#include "SheetsAnalyzer/Result.hpp"

namespace xlair::sheets::compilation {
    struct SliderPathPoint {
        enum class Kind { Start, End, Visible, Invisible, Control };
        Kind kind = Kind::Visible;
        TimelineIndex timeline = 0;
        s3d::int64 sample = 0;
        LaneSpan lane;
    };

    namespace detail {
        [[nodiscard]]
        Result<SliderHold> CompileSliderHold(
            const s3d::Array<SliderPathPoint>& points,
            s3d::int64 sample_rate,
            const s3d::Array<TempoChange>& tempo_changes
        );
    }
}
