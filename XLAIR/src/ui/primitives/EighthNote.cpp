#include "EighthNote.hpp"

namespace ui::primitives {
    void DrawEighthNote(const Vec2& pos, const double scale, const ColorF& color) {
        Circle{ pos + scale * Vec2{ 9.0, 31.0 }, scale * 9.0 }.draw(color);
        RectF{ pos + scale * Vec2{ 15.0, 0.0 }, scale * Vec2{ 15.0, 10.0 } }.draw(color);
        RectF{ pos + scale * Vec2{ 15.0, 0.0 }, scale * Vec2{ 3.0, 30.0 } }.draw(color);
    }
}