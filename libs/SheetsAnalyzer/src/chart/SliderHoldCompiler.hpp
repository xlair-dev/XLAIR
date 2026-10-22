#pragma once

#include "SheetsAnalyzer/Chart.hpp"
#include "SheetsAnalyzer/Result.hpp"

namespace xlair::sheets::chart {
    namespace detail {
        [[nodiscard]]
        Result<SliderHold> CompileSliderHold(
            const s3d::Array<SliderHoldPoint>& points,
            s3d::int64 sample_rate,
            const s3d::Array<TempoChange>& tempo_changes
        );
    }
}
