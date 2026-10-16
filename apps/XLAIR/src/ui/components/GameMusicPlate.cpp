#include "GameMusicPlate.hpp"

#include "ScrollingText.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/presentation/DifficultyText.hpp"
#include "ui/primitives/EighthNote.hpp"
#include "ui/theme/DifficultyTheme.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::components {
    void DrawGameMusicPlate(const GameMusicPlateData& data, const Point& position, const double elapsed) {
        const ScopedViewport2D viewport{ position, 430, 112 };
        const auto difficulty = presentation::DifficultyLabel(data.difficulty_index).uppercased();
        const auto colors = theme::GetDifficultyTheme(data.difficulty_index);

        Rect{ 0, 0, 112, 112 }(data.jacket).draw();
        Line{ 123, 76, 413, 76 }.draw(2, colors.accent);
        FontAsset{ assets::font::Text }(difficulty).draw(32, Arg::bottomLeft = Vec2{ 123, 77 }, colors.accent);

        constexpr Rect TitleRegion{ 123, 76, 260, 43 };
        DrawScrollingText(
            FontAsset{ assets::font::Text },
            data.title,
            30,
            TitleRegion.movedBy(position),
            elapsed,
            colors.secondary_text
        );

        {
            const Transformer2D transform{ Mat3x2::Rotate(-90_deg, Vec2{ 0, 0 }) };
            FontAsset{ assets::font::Label }(U"LEVEL").draw(23, Vec2{ -65, 293 }, colors.accent);
        }

        // Keep the legacy integer level display until a fractional level style is designed.
        const int32 level = static_cast<int32>(data.level);
        const double level_x = 325.0 + (level < 10 ? 10.0 : 0.0);
        FontAsset{ assets::font::Text }(level).drawBase(87, level_x, 63, colors.text);

        for (uint32 index = 0; index < data.max_plays; ++index) {
            const ColorF color = index < data.remaining_plays ? theme::Palette::Cyan : theme::Palette::Gray;
            primitives::DrawEighthNote(Vec2{ 123 + index * 20.0, 0 }, 0.675, color);
        }
    }
}
