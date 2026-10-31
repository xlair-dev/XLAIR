#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    inline constexpr double GameStartEnterDuration = 0.25;
    inline constexpr double GameStartMinimumDuration = 1.75;
    inline constexpr double GameStartRevealDuration = 0.35;

    struct GameStartData {
        StringView title;
        StringView artist;
        StringView designer;
        Texture jacket;
        uint32 difficulty_index = 0;
    };

    void DrawGameStart(const GameStartData& data, double elapsed, double reveal);
}
