#pragma once

#include "Common.hpp"
#include "core/judgement/JudgeEngine.hpp"

namespace xlair::ui::components {
    struct ResultPanelData {
        core::judgement::ScoreState score;
        std::size_t total_combo = 0;
        Optional<uint32> previous_rating;
        Optional<uint32> rating;
    };

    void DrawResultPanel(const ResultPanelData& data, const Point& position, double elapsed);
}
