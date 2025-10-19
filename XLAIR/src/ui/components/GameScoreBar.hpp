#pragma once
#include "Common.hpp"

#include "core/types/Scoring.hpp"

namespace ui::components {
    void DrawGameScoreBar(const Point& pos, const core::types::Score& score, const int32 width = 868);
}
