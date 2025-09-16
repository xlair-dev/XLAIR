#pragma once
#include "Palette.hpp"

#include "core/types/Difficulty.hpp"

namespace ui::theme {
    struct DifficultyTheme {
        ColorF accent;
        ColorF sub;
        ColorF text;
        ColorF sub_text;
    };

    inline DifficultyTheme GetDifficultyTheme(core::types::Difficulty difficulty) {
        using core::types::Difficulty;
        switch (difficulty) {
            case Difficulty::Basic: return {
                .accent = Palette::Cyan,
                .sub = Palette::Pink,
                .text = Palette::Gray,
                .sub_text = Palette::LightGray,
            };
            case Difficulty::Advanced: return {
                .accent = Palette::Pink,
                .sub = Palette::Cyan,
                .text = Palette::Gray,
                .sub_text = Palette::LightGray,
            };
            case Difficulty::Master: return {
                .accent = Palette::Purple,
                .sub = Palette::Cyan,
                .text = Palette::Gray,
                .sub_text = Palette::LightGray,
            };
            default: return {
                .accent = Palette::Gray,
                .sub = Palette::Gray,
                .text = Palette::Gray,
                .sub_text = Palette::LightGray,
            };
        }
    }
}