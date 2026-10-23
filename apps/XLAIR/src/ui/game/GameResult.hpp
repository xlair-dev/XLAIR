#pragma once

#include "core/judgement/JudgeEngine.hpp"
#include "ui/game/GameSelection.hpp"

namespace xlair::ui {
    // Own the result independently of the chart, audio, and judgement engine released by Game.
    struct GameResult {
        GameSelection selection;
        core::judgement::ScoreState score;
        std::size_t total_combo = 0;
    };
}
