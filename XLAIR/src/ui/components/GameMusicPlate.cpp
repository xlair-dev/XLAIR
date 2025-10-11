#include "GameMusicPlate.hpp"
#include "app/usecases/Assets.hpp"
#include "ui/primitives/EighthNote.hpp"
#include "ui/theme/Palette.hpp"
#include "ui/theme/DifficultyTheme.hpp"

namespace ui::components {
    void DrawGameMusicPlate(const Point& pos, const core::types::SheetMetadata& data, const TextureRegion& jacket, int8 difficulty_index, const double offset, int32 left_playable_music) {
        ScopedViewport2D viewport{ pos, 400, 112 };
        const auto difficulty = core::types::DifficultyFromInt(difficulty_index);
        const auto difficulty_info = data.difficulties[difficulty_index];
        const auto theme = theme::GetDifficultyTheme(difficulty);

        Rect{ 0, 0, 112, 112 }(jacket).draw();

        Line{ 123, 76, 383, 76 }.draw(2, theme.accent);

        FontAsset(app::assets::font::UiText)(U"MASTER").draw(38, Arg::bottomLeft = Vec2{123, 77}, theme.accent);
        FontAsset(app::assets::font::UiText)(data.title).draw(30, Arg::topLeft = Vec2{ 123, 72 }, theme.text);

        {
            const Transformer2D tr{ Mat3x2::Rotate(-90_deg, Vec2{0, 0}) };
            FontAsset(app::assets::font::UiLabel)(U"LEVEL").draw(23, Vec2{ -65, 265 }, theme.accent);
        }

        const auto level = difficulty_info.level;
        const double level_x = (level < 10 ? 305 : 295);
        FontAsset(app::assets::font::UiText)(level).drawBase(87, level_x, 63, theme.text);

        constexpr ColorF AccentColor = theme::Palette::Cyan;
        constexpr ColorF TextColor = theme::Palette::Gray;
        for (const auto i : step(3)) {
            const auto color = i < left_playable_music ? AccentColor : TextColor;
            primitives::DrawEighthNote(Vec2{ 123 + i * 20, 0 }, 0.675, color);
        }
    }
}
