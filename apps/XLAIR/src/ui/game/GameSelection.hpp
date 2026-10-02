#pragma once

#include "Common.hpp"

#include <SheetsAnalyzer/Metadata.hpp>

namespace xlair::ui {
    struct GameSelection {
        String music_id;
        String title;
        FilePath music_path;
        double music_offset_seconds = 0.0;
        sheets::Difficulty difficulty;
    };
}
