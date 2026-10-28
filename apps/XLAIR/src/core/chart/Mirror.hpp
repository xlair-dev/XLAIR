#pragma once

#include <SheetsAnalyzer/Chart.hpp>

namespace xlair::core::chart {
    // Reflect a validated 16-lane chart in place without changing its timing or note order.
    void ApplyMirror(sheets::Chart& chart);
}
