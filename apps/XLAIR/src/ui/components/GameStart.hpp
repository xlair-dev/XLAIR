#pragma once

#include "Common.hpp"
#include "ui/Design.hpp"

namespace xlair::ui::components {
    inline constexpr double GameStartEnterDuration = 0.25;
    inline constexpr double GameStartMinimumDuration = 1.75;
    inline constexpr double GameStartRevealDuration = 0.35;
    inline constexpr RectF GameStartJacketBounds{ (DesignSize.x - 400.0) / 2.0, 230.0, 400.0, 400.0 };

    struct GameStartData {
        StringView title;
        StringView artist;
        StringView designer;
        Texture jacket;
        uint32 difficulty_index = 0;
    };

    void DrawGameStartBackground(double elapsed, double reveal, double opacity = 1.0);

    void DrawGameStart(const GameStartData& data, double elapsed, double reveal, bool draw_jacket = true);
}
