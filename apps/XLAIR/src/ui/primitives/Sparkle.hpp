#pragma once

#include "Common.hpp"

namespace xlair::ui::primitives {
    [[nodiscard]]
    Shape2D Sparkle(
        const Vec2& center,
        double horizontal_radius,
        double vertical_radius,
        double angle = 0.0,
        double quality_scale = Graphics2D::GetMaxScaling()
    );
}
