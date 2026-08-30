#pragma once

#include <Siv3D.hpp>

#include "SheetsAnalyzer/Chart.hpp"
#include "SheetsAnalyzer/Result.hpp"

namespace xlair::sheets::formats::sus::detail {
    [[nodiscard]] Result<s3d::Array<s3d::int64>> GenerateHoldJudgeSamples(s3d::int64 start_sample,
                                                                          s3d::int64 end_sample, s3d::int64 sample_rate,
                                                                          const s3d::Array<TempoChange>& tempo_changes);
}
