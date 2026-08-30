#pragma once

#include <Siv3D.hpp>

#include "SheetsAnalyzer/Chart.hpp"
#include "SheetsAnalyzer/Result.hpp"
#include "Document.hpp"
#include "Timing.hpp"

namespace xlair::sheets::formats::sus::detail {
    [[nodiscard]] Result<s3d::Array<SliderHold>>
    CompileSliderHolds(const s3d::Array<::xlair::sheets::formats::sus::SliderHoldPoint>& source_points,
                       const TimingMap& timing, const s3d::HashTable<TimelineId, TimelineIndex>& timeline_lookup,
                       s3d::int64 sample_rate, const s3d::Array<TempoChange>& tempo_changes);
}
