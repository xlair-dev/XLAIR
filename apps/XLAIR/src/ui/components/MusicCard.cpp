#include "MusicCard.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/components/ScrollingText.hpp"
#include "ui/primitives/Sparkle.hpp"
#include <Siv3D/Math.hpp>

namespace xlair::ui::components {
    MusicCard::MusicCard() : m_render_texture{ CardSize } {}

    const MSRenderTexture& MusicCard::render(
        const MusicCardData& data,
        const TextureRegion& jacket,
        const theme::DifficultyTheme& difficulty_theme,
        const double text_offset
    ) {
        m_render_texture.clear(Palette::White);
        {
            const ScopedRenderTarget2D target{ m_render_texture };
            drawBackground(jacket, difficulty_theme);
            drawDifficulty(data, difficulty_theme);
            drawMetadata(data, difficulty_theme, text_offset);
        }
        Graphics2D::Flush();
        m_render_texture.resolve();
        return m_render_texture;
    }

    void MusicCard::drawBackground(const TextureRegion& jacket, const theme::DifficultyTheme& difficulty_theme) const {
        const ColorF corner_color = (difficulty_theme.accent * 1.5).withA(0.2);
        Circle{ 0, 0, 70 }.draw(corner_color, Palette::White);
        Circle{ CardSize.x, 0, 70 }.draw(corner_color, Palette::White);
        Circle{ 0, CardSize.y, 70 }.draw(corner_color, Palette::White);
        Circle{ CardSize.x, CardSize.y, 70 }.draw(corner_color, Palette::White);

        RectF{ CardSize }.drawFrame(2, 0, difficulty_theme.accent);

        RectF{ 58, 20, 338, 338 }.draw(difficulty_theme.accent);
        RectF{ 64, 20, 332, 332 }(jacket).draw();

        Line{ 349, 10, 406, 10 }.draw(2, difficulty_theme.accent);
        Line{ 406, 10, 406, 67 }.draw(2, difficulty_theme.accent);
        Line{ 406, 496, 406, 534 }.draw(2, difficulty_theme.accent);

        for (const double y : { 365.0, 420.0, 474.0 }) {
            primitives::Sparkle(Vec2{ 30, y }, 18, 25).draw(difficulty_theme.accent);
        }
    }

    void MusicCard::drawDifficulty(const MusicCardData& data, const theme::DifficultyTheme& difficulty_theme) const {
        {
            const Transformer2D transform{ Mat3x2::Rotate(90_deg) };
            FontAsset{ assets::font::Label }(String{ data.difficulty }.uppercased())
                .draw(36, Vec2{ 20, -57 }, difficulty_theme.accent);
        }

        {
            const Transformer2D transform{ Mat3x2::Rotate(-90_deg) };
            FontAsset{ assets::font::Label }(U"LEVEL").draw(23, Vec2{ -505, 50 }, difficulty_theme.accent);
        }

        const int32 level = static_cast<int32>(Math::Round(data.level));
        const double level_x = level < 10 ? 90 : 80;
        FontAsset{ assets::font::Text }(level).drawBase(87, level_x, 503, difficulty_theme.text);
    }

    void MusicCard::drawMetadata(
        const MusicCardData& data,
        const theme::DifficultyTheme& difficulty_theme,
        const double text_offset
    ) const {
        const Font font = FontAsset{ assets::font::Text };
        DrawScrollingText(font, data.title, 36, Rect{ 58, 363, 338, 50 }, text_offset, difficulty_theme.text, true);
        DrawScrollingText(
            font,
            data.artist,
            24,
            Rect{ 58, 363, 338, 75 },
            text_offset,
            difficulty_theme.secondary_text,
            true
        );
        constexpr double DesignerX = 16;
        constexpr double DesignerBaselineY = 534;
        constexpr double DesignerFontSize = 15;
        constexpr double DesignerLineRight = 406;
        constexpr double DesignerLineGap = 8;

        const String designer = U"NOTES DESIGNER: " + data.designer;
        const auto designer_text = font(designer);
        designer_text.drawBase(DesignerFontSize, DesignerX, DesignerBaselineY, difficulty_theme.accent);

        const double line_left =
            Min(DesignerX + designer_text.region(DesignerFontSize).w + DesignerLineGap, DesignerLineRight);
        if (line_left < DesignerLineRight) {
            Line{ line_left, DesignerBaselineY, DesignerLineRight, DesignerBaselineY }.draw(2, difficulty_theme.accent);
        }
    }
}
