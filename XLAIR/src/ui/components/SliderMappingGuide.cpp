#include "SliderMappingGuide.hpp"
#include "app/consts/Scene.hpp"
#include "app/usecases/Assets.hpp"

namespace ui::components {
    void DrawSliderMappingGuide(const Array<Mapping>& mapping) {
        constexpr double w = Width / 16.0;
        constexpr double dw = w * 0.25;
        const auto slider_left = app::consts::SceneWidth / 2.0 - Width / 2.0;
        const auto top = app::consts::SceneHeight - 70;
        const auto bottom = app::consts::SceneHeight;
        for (const auto& map : mapping) {
            const auto left = slider_left + map.region.start * w + 2;
            const auto right = slider_left + (map.region.start + map.region.width) * w - 2;
            const auto top_left = left + (map.region.left_corner ? dw : 0);
            const auto top_right = right - (map.region.right_corner ? dw : 0);
            Quad{
                Vec2{ top_left, top },
                Vec2{ top_right, top },
                Vec2{ right, bottom },
                Vec2{ left, bottom },
            }.draw(
                map.color.withA(0.5),
                map.color.withA(0.5),
                map.color.withA(0.0),
                map.color.withA(0.0)
            );
            Line{
                Vec2{ top_left + 1, top },
                Vec2{ top_right - 1, top },
            }.draw(2, map.color);

            FontAsset(app::assets::font::UiText)(map.label).drawAt(
                22,
                Vec2{ (left + right) / 2.0, (top + bottom) / 2.0 },
                map.color
            );
        }
    }
}
