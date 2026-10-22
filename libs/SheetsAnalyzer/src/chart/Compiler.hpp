#pragma once

#include <chart_converter.h>
#include "SheetsAnalyzer.hpp"

namespace xlair::sheets::chart {
    [[nodiscard]]
    Result<Chart> Compile(const CcResult& parsed, const ChartLoadOptions& options);
}
