#include "Arrow.hpp"

namespace ui::primitives {
    namespace {
        static const Polygon RightArrow{
            Vec2{ 0, -47 }, Vec2{ 47, 0 }, Vec2{ 0, 47 }, Vec2{ 0, 26 }, Vec2{ 26, 0 }, Vec2{ 0, -26 },
        };
        static const Polygon LeftArrow{
            Vec2{ 0, -47 }, Vec2{ 0, -26 }, Vec2{ -26, 0 }, Vec2{ 0, 26 }, Vec2{ 0, 47 }, Vec2{ -47, 0 },
        };
    }

    void DrawArrow(const Vec2& pos, const ColorF& color, int8 dir) {
        (dir > 0 ? RightArrow : LeftArrow).movedBy(pos).draw(color);
    }
}