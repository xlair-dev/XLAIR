#pragma once
#include "Common.hpp"
#include "ui/theme/Palette.hpp"

namespace ui::components {
    struct Mapping {
        struct Region {
            uint8 start;
            uint8 width;
            bool left_corner = true;
            bool right_corner = true;
        } region;
        String label;
        ColorF color;
    };

    static inline constexpr double Width = 1500;

    void DrawSliderMappingGuide(const Array<Mapping>& mapping);
}
