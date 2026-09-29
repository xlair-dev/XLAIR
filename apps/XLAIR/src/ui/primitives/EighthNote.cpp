#include "EighthNote.hpp"

namespace xlair::ui::primitives {
    void DrawEighthNote(const Vec2& position, const double scale, const ColorF& color) {
        Circle{ position + scale * Vec2{ 9, 31 }, scale * 9 }.draw(color);
        RectF{ position + scale * Vec2{ 15, 0 }, scale * Vec2{ 15, 10 } }.draw(color);
        RectF{ position + scale * Vec2{ 15, 0 }, scale * Vec2{ 3, 30 } }.draw(color);
    }
}
