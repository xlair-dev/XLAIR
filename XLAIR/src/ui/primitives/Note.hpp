#pragma once
#include "Common.hpp"

namespace ui::primitives {
    void DrawTapNote(const Vec2& left_center, double width, double height = 32.0);
    void DrawXTapNote(const Vec2& left_center, double width, double height = 32.0);
    void DrawFlickNote(const Vec2& left_center, double width, double height = 32.0);
    void DrawHoldNoteHead(const Vec2& left_center, double width, double height = 32.0);
    void DrawHoldBack(const Vec2& start_left, double start_width, double start_t, const Vec2& end_left, double end_width, double end_t);
}
