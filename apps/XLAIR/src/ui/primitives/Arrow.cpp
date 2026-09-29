#include "Arrow.hpp"

namespace xlair::ui::primitives {
    namespace {
        const Polygon RightArrow{
            Vec2{ 0, -47 }, Vec2{ 47, 0 }, Vec2{ 0, 47 }, Vec2{ 0, 26 }, Vec2{ 26, 0 }, Vec2{ 0, -26 },
        };

        const Polygon LeftArrow{
            Vec2{ 0, -47 }, Vec2{ 0, -26 }, Vec2{ -26, 0 }, Vec2{ 0, 26 }, Vec2{ 0, 47 }, Vec2{ -47, 0 },
        };
    }

    void DrawArrow(const Vec2& center, const ArrowDirection direction, const ColorF& color, const double scale) {
        const auto& arrow = direction == ArrowDirection::Right ? RightArrow : LeftArrow;
        arrow.scaled(scale).movedBy(center).draw(color);
    }
}
