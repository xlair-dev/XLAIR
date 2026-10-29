#pragma once

#include "Timing.hpp"

namespace xlair::sheets::chart {
    [[nodiscard]]
    Result<s3d::Array<MeasureLine>> CompileMeasureLines(const CcResult& parsed, const TimingMap& timing);
}
