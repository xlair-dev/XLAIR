#include "UserNameplate.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::components {
    void DrawUserNameplate(const UserNameplateData& data, const Point& position) {
        const ScopedViewport2D viewport{ position, 600, 200 };

        constexpr ColorF BaseColor = theme::Palette::Cyan;
        constexpr ColorF AccentColor = BaseColor.withA(0.44);
        constexpr ColorF SubColor{ AccentColor, 0.16 };
        constexpr ColorF TextColor = theme::Palette::Gray;

        constexpr double DiamondSize = 70;
        constexpr Vec2 Center{ DiamondSize, DiamondSize };
        constexpr Vec2 Top{ DiamondSize, 0 };
        constexpr Vec2 Left{ 0, DiamondSize };
        constexpr Vec2 Bottom = Center + Vec2{ 0, DiamondSize };
        constexpr Vec2 Right = Center + Vec2{ DiamondSize, 0 };

        Triangle{ Center, Top, Left }.draw(AccentColor, SubColor, SubColor);
        Triangle{ Center, Left, Bottom }.draw(AccentColor, SubColor, SubColor);
        Triangle{ Center, Bottom, Right }.draw(AccentColor, SubColor, SubColor);
        Triangle{ Center, Right, Top }.draw(AccentColor, SubColor, SubColor);
        Quad{ Top, Left, Bottom, Right }.scaledAt(Center, 0.65).draw(Palette::White);

        FontAsset{ assets::font::Text }(data.level).drawAt(40, Center.movedBy(0, 2), BaseColor);

        constexpr double GaugeWidth = 336;
        constexpr Vec2 GaugeStart{ 35, 115 };
        constexpr Vec2 GaugeEnd = GaugeStart + Vec2{ GaugeWidth, 0 };
        const double progress = Math::Clamp(data.level_progress, 0.0, 1.0);
        Line{ GaugeStart, GaugeEnd }.draw(LineStyle::RoundCap, 4, AccentColor);
        Line{ GaugeStart, GaugeStart.movedBy(GaugeWidth * progress, 0) }.draw(LineStyle::RoundCap, 4, BaseColor);

        FontAsset{ assets::font::Text }(data.display_name).draw(32, Arg::bottomLeft = Right.movedBy(5, 10), TextColor);

        FontAsset{ assets::font::Label }(U"RATE").draw(18, Arg::topLeft = Right.movedBy(5, 6), BaseColor);
        FontAsset{ assets::font::Text }(data.rating).draw(18, Arg::topLeft = Right.movedBy(57, 6), TextColor);
        Triangle{ 0, 0, 0, 11, 5.5, 5.5 }.movedBy(Right).movedBy(50, 13.5).draw(AccentColor);
    }
}
