#include "MenuTimerPlate.hpp"
#include "ui/primitives/EighthNote.hpp"
#include "ui/theme/Palette.hpp"

namespace ui::components {
    void DrawMenuTimerPlate(const Point& pos, int32 time, int32 left_playable_music) {
        ScopedViewport2D viewport{ pos, 400, 200 };
        constexpr ColorF AccentColor = theme::Palette::Cyan;
        constexpr ColorF TextColor = theme::Palette::Gray;

        Line { 0, 88, 249, 88 }.draw(LineStyle::RoundCap, 4, AccentColor);

        FontAsset(U"tile.text")(U"TIME").drawBase(32, 13, 78, AccentColor);
        FontAsset(U"BrunoAce")(time).draw(68, Arg::bottomLeft = Vec2{ 100, 96 }, TextColor);

        for (const auto i : step(3)) {
            const auto color = i < left_playable_music ? AccentColor : TextColor;
            primitives::DrawEighthNote(Vec2{ 5 + i * 42, 100 }, 1.0, color);
        }
    }
}
