#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    struct GameMusicPlateData {
        StringView title;
        TextureRegion jacket;
        uint32 difficulty_index = 0;
        double level = 0.0;
        uint32 max_plays = 0;
        uint32 remaining_plays = 0;
    };

    struct GameMusicPlateLabelData {
        StringView heading;
        StringView title;
    };

    void DrawGameMusicPlate(const GameMusicPlateData& data, const Point& position, double elapsed);

    void DrawGameMusicPlate(const GameMusicPlateLabelData& data, const Point& position, double elapsed);
}
