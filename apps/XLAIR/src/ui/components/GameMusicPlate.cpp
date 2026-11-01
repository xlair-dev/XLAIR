#include "GameMusicPlate.hpp"

#include "DifficultyLevel.hpp"
#include "ScrollingText.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/presentation/DifficultyText.hpp"
#include "ui/primitives/EighthNote.hpp"
#include "ui/theme/DifficultyTheme.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::components {
    namespace {
        void DrawPlateText(
            const GameMusicPlateLabelData& data,
            const Rect& title_region,
            const Point& position,
            const double elapsed,
            const ColorF& accent,
            const ColorF& title_color
        ) {
            Line{ title_region.x, 76, 413, 76 }.draw(2, accent);
            FontAsset{ assets::font::Text }(data.heading)
                .draw(32, Arg::bottomLeft = Vec2{ title_region.x, 77 }, accent);
            DrawScrollingText(
                FontAsset{ assets::font::Text },
                data.title,
                30,
                title_region.movedBy(position),
                elapsed,
                title_color
            );
        }
    }

    void DrawGameMusicPlate(const GameMusicPlateData& data, const Point& position, const double elapsed) {
        const ScopedViewport2D viewport{ position, 430, 112 };
        const auto difficulty = presentation::DifficultyLabel(data.difficulty_index).uppercased();
        const auto colors = theme::GetDifficultyTheme(data.difficulty_index);

        Rect{ 0, 0, 112, 112 }(data.jacket).draw();
        DrawPlateText(
            { .heading = difficulty, .title = data.title },
            Rect{ 123, 76, 260, 43 },
            position,
            elapsed,
            colors.accent,
            colors.secondary_text
        );

        {
            const Transformer2D transform{ Mat3x2::Rotate(-90_deg, Vec2{ 0, 0 }) };
            FontAsset{ assets::font::Label }(U"LEVEL").draw(23, Vec2{ -65, 293 }, colors.accent);
        }

        const double level_x = 325.0 + (data.level < 10.0 ? 10.0 : 0.0);
        DrawDifficultyLevel(data.level, Vec2{ level_x, 63 }, 423.0 - level_x, colors.text);

        for (uint32 index = 0; index < data.max_plays; ++index) {
            const ColorF color = index < data.remaining_plays ? theme::Palette::Cyan : theme::Palette::Gray;
            primitives::DrawEighthNote(Vec2{ 123 + index * 20.0, 0 }, 0.675, color);
        }
    }

    void DrawGameMusicPlate(const GameMusicPlateLabelData& data, const Point& position, const double elapsed) {
        const ScopedViewport2D viewport{ position, 430, 112 };
        DrawPlateText(data, Rect{ 0, 76, 413, 43 }, position, elapsed, theme::Palette::Cyan, theme::Palette::LightGray);
    }
}
