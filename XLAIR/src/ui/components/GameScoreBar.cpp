#include "GameScoreBar.hpp"
#include "app/usecases/Assets.hpp"
#include "ui/theme/Palette.hpp"
#include "ui/addons/TextShadowAddon.hpp"

namespace ui::components {
    void DrawGameScoreBar(const Point& pos, const core::types::Score& score, const int32 width) {
        ScopedViewport2D viewport{ pos, width, 150 };
        auto&& font = FontAsset(app::assets::font::UiSubText);

        font(U"SCORE").drawBase(32, 32, 42, theme::Palette::Gray);

        addons::TextShadowAddon::DrawAt(U"{:0>8}"_fmt(score.score_value), font, 48, Vec2{ width / 2.0, 30 }, theme::Palette::White, theme::Palette::Darkblue, Vec2{ 1, 1 });

        TextureAsset(app::assets::texture::GameScoreBarFrame).resized(width, 67).draw(0, 49);
    }
} 
