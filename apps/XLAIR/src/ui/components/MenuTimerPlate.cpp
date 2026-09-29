#include "MenuTimerPlate.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/primitives/EighthNote.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::components {
    void DrawMenuTimerPlate(const MenuTimerPlateData& data, const Point& position) {
        const ScopedViewport2D viewport{ position, 400, 200 };

        constexpr ColorF AccentColor = theme::Palette::Cyan;
        constexpr ColorF TextColor = theme::Palette::Gray;

        Line{ 0, 88, 249, 88 }.draw(LineStyle::RoundCap, 4, AccentColor);

        FontAsset{ assets::font::Text }(U"TIME").drawBase(32, 13, 78, AccentColor);
        FontAsset{ assets::font::Display }(Max(data.remaining_seconds, 0))
            .draw(68, Arg::bottomLeft = Vec2{ 100, 96 }, TextColor);

        for (uint32 index = 0; index < data.max_plays; ++index) {
            const ColorF color = index < data.remaining_plays ? AccentColor : TextColor;
            primitives::DrawEighthNote(Vec2{ 5 + index * 42.0, 100 }, 1.0, color);
        }
    }
}
