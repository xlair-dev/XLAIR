#include "SliderMappingGuide.hpp"

#include "ui/assets/Assets.hpp"

namespace xlair::ui::components {
    void DrawSliderMappingGuide(const Array<SliderMapping>& mappings, const RectF& region, const uint32 column_count) {
        if (column_count == 0) {
            return;
        }

        const double column_width = region.w / column_count;
        const double corner_width = column_width * 0.25;
        for (const auto& mapping : mappings) {
            if (mapping.region.width == 0 || mapping.region.start >= column_count) {
                continue;
            }

            const uint32 end = mapping.region.start + Min(mapping.region.width, column_count - mapping.region.start);
            const double left = region.x + mapping.region.start * column_width + 2;
            const double right = region.x + end * column_width - 2;
            const double top_left = left + (mapping.region.left_corner ? corner_width : 0);
            const double top_right = right - (mapping.region.right_corner ? corner_width : 0);

            Quad{
                Vec2{ top_left, region.y },
                Vec2{ top_right, region.y },
                Vec2{ right, region.bottomY() },
                Vec2{ left, region.bottomY() },
            }
                .draw(
                    mapping.color.withA(0.5),
                    mapping.color.withA(0.5),
                    mapping.color.withA(0.0),
                    mapping.color.withA(0.0)
                );
            Line{ top_left + 1, region.y, top_right - 1, region.y }.draw(2, mapping.color);
            FontAsset{ assets::font::Text }(mapping.label)
                .drawAt(22, Vec2{ (left + right) / 2.0, region.centerY() }, mapping.color);
        }
    }
}
