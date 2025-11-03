#include "GameScoreBar.hpp"
#include "app/usecases/Assets.hpp"
#include "ui/theme/Palette.hpp"
#include "ui/addons/TextShadowAddon.hpp"

namespace ui::components {
    void DrawGameScoreBar(const Point& pos, const core::types::Score& score, const int32 width) {
        ScopedViewport2D viewport{ pos, width, 150 };
        auto&& font = FontAsset(app::assets::font::UiSubText);

        font(U"SCORE").drawBase(32, 32, 42, theme::Palette::Gray);

        addons::TextShadowAddon::DrawAt(U"{:0>7}"_fmt(score.score_value), font, 48, Vec2{ width / 2.0, 30 }, theme::Palette::White, theme::Palette::Darkblue, Vec2{ 1, 1 });

        TextureAsset(app::assets::texture::GameScoreBarFrame).resized(width, 67).draw(0, 49);

        //Line{ 22.0, 49 + 67 / 2.0, width - 22.0 }
        const auto x = 40.0 + (width - 80.0) * score.clear_guage;
        const auto guage_center = Vec2{ x, 49 + 67 / 2.0 };
        const ColorF color = (score.clear_guage >= 0.7) ? theme::Palette::Pink : theme::Palette::Cyan;

        Line{ 40.0, guage_center.y, guage_center.x, guage_center.y }
            .draw(LineStyle::RoundCap, 6.0, color, color.withA(0.53));

        const auto t1 = guage_center.movedBy(0, -35);
        const auto t2 = guage_center.movedBy(35, 0);
        const auto t3 = guage_center.movedBy(0, 35);
        const auto t4 = guage_center.movedBy(-35, 0);
        Quad{ t1, t2, t3, t4 }.draw(color.withA(0.6));
        Quad{ t1, t2, t3, t4 }.scaledAt(guage_center, 0.5).draw(theme::Palette::White);
    }
} 
