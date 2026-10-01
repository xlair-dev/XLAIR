#pragma once

#include "ui/primitives/Arrow.hpp"

namespace xlair::ui::primitives {
    // Draws the four overlapping gradient layers and white chevron from the Figma arrow.
    void DrawSettingValueArrow(
        const Vec2& center,
        ArrowDirection direction,
        const ColorF& primary,
        const ColorF& secondary,
        double scale = 1.0
    );
}
