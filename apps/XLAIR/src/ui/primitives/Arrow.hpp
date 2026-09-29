#pragma once

#include "Common.hpp"

namespace xlair::ui::primitives {
    enum class ArrowDirection {
        Left,
        Right,
    };

    void DrawArrow(const Vec2& center, ArrowDirection direction, const ColorF& color, double scale = 1.0);
}
