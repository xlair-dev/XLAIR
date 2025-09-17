#include "PlayerNameplate.hpp"
#include "ui/theme/Palette.hpp"

namespace ui::components {
    void DrawPlayerNameplate(const core::types::PlayerData& data, const Point& pos) {
        // TODO: user
        ScopedViewport2D viewport{ pos, 600, 200 };
        constexpr ColorF BaseColor = theme::Palette::Cyan;
        constexpr ColorF AccentColor = BaseColor.withA(0.44);
        constexpr ColorF SubColor = ColorF{ AccentColor, 0.16 };
        constexpr ColorF TextColor = theme::Palette::Gray;

        constexpr double DiamondSize = 70.0;
        const auto center = Vec2{ DiamondSize, DiamondSize };
        const auto t1 = Vec2{ DiamondSize, 0 };
        const auto t2 = Vec2{ 0, DiamondSize };
        const auto t3 = center + Vec2{ 0, DiamondSize };
        const auto t4 = center + Vec2{ DiamondSize, 0 };
        Triangle{ center, t1, t2 }.draw(AccentColor, SubColor, SubColor);
        Triangle{ center, t2, t3 }.draw(AccentColor, SubColor, SubColor);
        Triangle{ center, t3, t4 }.draw(AccentColor, SubColor, SubColor);
        Triangle{ center, t4, t1 }.draw(AccentColor, SubColor, SubColor);

        Quad{ t1, t2, t3, t4 }.scaledAt(center, 0.65).draw(Palette::White);

        // level
        FontAsset(U"tile.text")(data.level).drawAt(40, center.movedBy(0, 2), BaseColor);

        // level guage
        constexpr double GuageWidth = 336;
        Line{ 35, 115, 35 + GuageWidth, 115 }.draw(LineStyle::RoundCap, 4, AccentColor);
        Line{ 35, 115, 35 + 179, 115 }.draw(LineStyle::RoundCap, 4, BaseColor); // TODO: handle user level

        // name
        FontAsset(U"tile.text")(data.displayname).draw(32, Arg::bottomLeft = t4.movedBy(5, 10), TextColor);

        // rating
        FontAsset(U"tile")(U"RATE").draw(18, Arg::topLeft = t4.movedBy(5, 6), BaseColor);
        FontAsset(U"tile.text")(data.rating).draw(18, Arg::topLeft = t4.movedBy(57, 6), TextColor);
        Triangle{ 0, 0, 0, 11, 5.5, 5.5 }.movedBy(t4).movedBy(50, 13.5).draw(AccentColor);
    }
}
