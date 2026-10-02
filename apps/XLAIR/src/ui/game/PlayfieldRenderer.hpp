#pragma once

#include "Common.hpp"

#include <Playfield.hpp>
#include <SheetsAnalyzer.hpp>

namespace xlair::ui::game {
    class PlayfieldRenderer {
    public:
        void draw(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            int64 current_sample,
            const Rect& viewport,
            double pixels_per_second
        ) const;
    };
}
