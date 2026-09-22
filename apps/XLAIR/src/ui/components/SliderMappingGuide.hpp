#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    struct SliderMapping {
        struct Region {
            uint32 start = 0;
            uint32 width = 1;
            bool left_corner = true;
            bool right_corner = true;
        } region;

        String label;
        ColorF color;
    };

    void DrawSliderMappingGuide(const Array<SliderMapping>& mappings, const RectF& region, uint32 column_count = 16);
}
