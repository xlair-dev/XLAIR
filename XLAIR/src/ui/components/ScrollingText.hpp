#pragma once
#include "Common.hpp"

namespace ui::components {
    inline void DrawScrollingText(const Font& font, StringView text, double size, const Rect& region, double offset, const ColorF& color, bool centerize = false, double speed = 50.0) {
        const ScopedViewport2D viewport{ region };
        const RectF text_region = font(text).region(size);
        const double text_margin = size * 2.2;

        if (text_region.w <= region.w) {
            if (centerize) {
                const Vec2 pos{ region.w / 2.0, region.h };
                font(text).draw(size, Arg::bottomCenter = pos, color);
            } else {
                const Vec2 pos{ 0.0, region.h };
                font(text).draw(size, Arg::bottomLeft = pos, color);
            }
        } else {
            const double t = Math::Fmod(offset, (text_region.w + text_margin) / speed);
            font(text).draw(size, Arg::bottomLeft = Vec2{ -t * speed, region.h }, color);
            font(text).draw(size, Arg::bottomLeft = Vec2{ -t * speed + text_region.w + text_margin, region.h }, color);
        }
    }
}
