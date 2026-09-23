#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    inline void DrawScrollingText(
        const Font& font,
        const StringView text,
        const double font_size,
        const Rect& region,
        const double elapsed,
        const ColorF& color,
        const bool centered = false,
        const double speed = 50.0
    ) {
        const ScopedViewport2D viewport{ region };
        const RectF text_region = font(text).region(font_size);
        if (text_region.w <= region.w) {
            const Vec2 position{
                centered ? region.w / 2.0 : 0.0,
                static_cast<double>(region.h),
            };
            if (centered) {
                font(text).draw(font_size, Arg::bottomCenter = position, color);
            } else {
                font(text).draw(font_size, Arg::bottomLeft = position, color);
            }
            return;
        }

        const double margin = font_size * 2.2;
        const double cycle_width = text_region.w + margin;
        const double offset = Math::Fmod(Math::Max(elapsed, 0.0) * speed, cycle_width);
        const Vec2 origin{ -offset, static_cast<double>(region.h) };
        font(text).draw(font_size, Arg::bottomLeft = origin, color);
        font(text).draw(font_size, Arg::bottomLeft = origin.movedBy(cycle_width, 0), color);
    }
}
