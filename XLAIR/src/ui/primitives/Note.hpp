#pragma once
#include "Common.hpp"

namespace ui::primitives {
    void DrawTapNote(const Vec2& left_center, double width, double height = 32.0);
    void DrawXTapNote(const Vec2& left_center, double width, double height = 32.0);
    void DrawFlickNote(const Vec2& left_center, double width, double height = 32.0);
    void DrawHoldNoteHead(const Vec2& left_center, double width, double height = 32.0);
}
