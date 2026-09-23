#pragma once

#include "Common.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::theme {
    struct DifficultyTheme {
        ColorF accent;
        ColorF secondary;
        ColorF text;
        ColorF secondary_text;
    };

    [[nodiscard]]
    inline DifficultyTheme GetDifficultyTheme(const uint32 index) noexcept {
        switch (index) {
            case 0:
                return {
                    .accent = Palette::Cyan,
                    .secondary = Palette::Pink,
                    .text = Palette::Gray,
                    .secondary_text = Palette::LightGray,
                };
            case 1:
                return {
                    .accent = Palette::Pink,
                    .secondary = Palette::Cyan,
                    .text = Palette::Gray,
                    .secondary_text = Palette::LightGray,
                };
            case 2:
                return {
                    .accent = Palette::Purple,
                    .secondary = Palette::Cyan,
                    .text = Palette::Gray,
                    .secondary_text = Palette::LightGray,
                };
            default:
                return {
                    .accent = Palette::DimmedPurple,
                    .secondary = Palette::Cyan,
                    .text = Palette::Gray,
                    .secondary_text = Palette::LightGray,
                };
        }
    }
}
