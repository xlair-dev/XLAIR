#pragma once

#include "Common.hpp"

#include <SheetsAnalyzer/Metadata.hpp>

namespace xlair::ui {
    struct GameSelection {
        UUIDValue play_id;
        uint32 max_plays = 0;
        uint32 remaining_plays = 0;
        bool autoplay = false;
        bool mirror = false;
        String music_id;
        String title;
        FilePath music_path;
        double music_offset_seconds = 0.0;
        sheets::Difficulty difficulty;
    };
}
