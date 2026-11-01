#pragma once

#include "Common.hpp"

namespace xlair::ui::transitions {
    inline constexpr int32 MusicStartMillisec = 500;

    struct MusicStart {
        Texture jacket;
        RectF source_region;
        ColorF source_background{ 0.05 };

        [[nodiscard]]
        RectF boundsAt(double progress) const;

        [[nodiscard]]
        RectF imageRegionAt(double progress) const;

        void draw(double progress) const;
    };
}
