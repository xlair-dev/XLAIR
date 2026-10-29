#include "ScreenWipe.hpp"

#include "ui/theme/Palette.hpp"

namespace xlair::ui::transitions {
    ScreenWipeGeometry MakeScreenWipe(const double progress, const SizeF& size) {
        constexpr double Overscan = 2.0;
        const double slant = Tan(18_deg) * (size.y / 2.0 + Overscan);
        // Oversize the band so the scene swap at progress 0.5 stays fully covered.
        const double margin = (size.x + size.y) * 0.2;
        const double width = size.x + slant * 2.0 + margin * 2.0;
        const double left =
            Math::Lerp(size.x + slant + Overscan, -width - slant - Overscan, EaseInOutCubic(Clamp(progress, 0.0, 1.0)));
        const auto band = [&](const double x, const double band_width) {
            return Quad{
                Vec2{ x - slant, -Overscan },
                Vec2{ x + band_width - slant, -Overscan },
                Vec2{ x + band_width + slant, size.y + Overscan },
                Vec2{ x + slant, size.y + Overscan },
            };
        };
        return {
            .background = band(left, width),
            .accent = band(left + width / 2.0, width / 2.0),
        };
    }

    void DrawScreenWipe(const double progress) {
        const auto geometry = MakeScreenWipe(progress, SizeF{ Scene::Size() });
        geometry.background.draw(theme::Palette::Cyan);
        geometry.accent.draw(theme::Palette::Pink);
    }
}
