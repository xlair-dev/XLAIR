#pragma once
#include "Common.hpp"

namespace ui::primitives {
    Shape2D Sparkle(const Vec2& center, const double a, const double b, const double angle, const double quality_scale = Graphics2D::GetMaxScaling());
}
