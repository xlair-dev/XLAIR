#pragma once

#include <Playfield.hpp>
#include <SheetsAnalyzer.hpp>

namespace xlair::sheets_viewer {
    class AnalysisRenderer {
    public:
        void draw(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            s3d::int64 current_sample,
            const s3d::Rect& viewport,
            double pixels_per_second
        ) const;
    };
}
