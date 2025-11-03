#pragma once
#include "Common.hpp"
#include "core/types/Sheet.hpp"

namespace ui::components {
    void DrawGameMusicPlate(const Point& pos, const core::types::SheetMetadata& data, const TextureRegion& jacket, int8 difficulty_index, const double offset, int32 max_playable_music, int32 left_playable_music);
}
