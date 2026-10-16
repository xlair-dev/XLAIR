#pragma once

#include "SheetsAnalyzer.hpp"
#include <chart_converter.h>

namespace xlair::sheets::compilation {
    /// Use a successful ChartConverter result's view, with its validated IDs, lanes, and paths.
    /// The view must remain valid for the duration of this call.
    [[nodiscard]]
    Result<Chart> Compile(const CcChartView& source, const ChartLoadOptions& options);
}
