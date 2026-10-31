#pragma once

#include "Common.hpp"

#include <array>

namespace xlair::ui::presentation {
    [[nodiscard]]
    inline String DifficultyLevelText(const double level) {
        const double integer = Math::Floor(level);
        return U"{:.0f}{}"_fmt(integer, level - integer >= 0.5 ? U"+" : U"");
    }

    [[nodiscard]]
    inline String DifficultyLabel(const uint32 index) {
        constexpr std::array Labels{
            StringView{ U"Basic" },
            StringView{ U"Advanced" },
            StringView{ U"Master" },
        };
        return index < Labels.size() ? String{ Labels[index] } : U"Difficulty {}"_fmt(index + 1);
    }
}
