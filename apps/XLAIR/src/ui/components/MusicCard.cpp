#include "MusicCard.hpp"

#include "DifficultyLevel.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/components/ScrollingText.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/primitives/Sparkle.hpp"

namespace xlair::ui::components {
    bool MusicCard::CacheKey::matches(
        const MusicCardData& data,
        const TextureRegion& image,
        const theme::DifficultyTheme& colors,
        const double offset,
        const bool show_jacket
    ) const {
        return title == data.title && artist == data.artist && difficulty == data.difficulty && level == data.level &&
               designer == data.designer && high_score == data.high_score && grade == data.grade &&
               clear_status == data.clear_status && available == data.available && jacket.texture == image.texture &&
               jacket.uvRect.left == image.uvRect.left && jacket.uvRect.top == image.uvRect.top &&
               jacket.uvRect.right == image.uvRect.right && jacket.uvRect.bottom == image.uvRect.bottom &&
               jacket.size == image.size && theme.accent == colors.accent &&
               theme.secondary == colors.secondary && theme.text == colors.text &&
               theme.secondary_text == colors.secondary_text && text_offset == offset && draw_jacket == show_jacket;
    }

    MusicCard::MusicCard() : m_render_texture{ CardSize } {}

    const MSRenderTexture& MusicCard::render(
        const MusicCardData& data,
        const TextureRegion& jacket,
        const theme::DifficultyTheme& difficulty_theme,
        double text_offset,
        const bool draw_jacket
    ) {
        // Elapsed time only affects cards with text wider than the scrolling viewport.
        const Font font = FontAsset{ assets::font::Text };
        if (font(data.title).region(36).w <= 338 && font(data.artist).region(24).w <= 338) {
            text_offset = 0.0;
        }
        if (m_cache && m_cache->matches(data, jacket, difficulty_theme, text_offset, draw_jacket)) {
            return m_render_texture;
        }
        m_cache = CacheKey{
            String{ data.title }, String{ data.artist }, String{ data.difficulty }, data.level,
            String{ data.designer }, data.high_score, String{ data.grade }, String{ data.clear_status },
            data.available, jacket, difficulty_theme, text_offset, draw_jacket
        };
        m_render_texture.clear(Palette::White);
        {
            const ScopedRenderTarget2D target{ m_render_texture };
            drawBackground(jacket, difficulty_theme, draw_jacket);
            drawDifficulty(data, difficulty_theme);
            drawMetadata(data, difficulty_theme, text_offset);
            if (!data.available) {
                drawUnavailable();
            }
        }
        Graphics2D::Flush();
        m_render_texture.resolve();
        return m_render_texture;
    }

    RectF MusicCard::jacketRegion(const RectF& card_region) noexcept {
        const double scale_x = card_region.w / CardSize.x;
        const double scale_y = card_region.h / CardSize.y;
        return {
            card_region.x + JacketRegion.x * scale_x,
            card_region.y + JacketRegion.y * scale_y,
            JacketRegion.w * scale_x,
            JacketRegion.h * scale_y,
        };
    }

    void MusicCard::drawBackground(
        const TextureRegion& jacket,
        const theme::DifficultyTheme& difficulty_theme,
        const bool draw_jacket
    ) const {
        const ColorF corner_color = (difficulty_theme.accent * 1.5).withA(0.2);
        Circle{ 0, 0, 70 }.draw(corner_color, Palette::White);
        Circle{ CardSize.x, 0, 70 }.draw(corner_color, Palette::White);
        Circle{ 0, CardSize.y, 70 }.draw(corner_color, Palette::White);
        Circle{ CardSize.x, CardSize.y, 70 }.draw(corner_color, Palette::White);

        RectF{ CardSize }.drawFrame(2, 0, difficulty_theme.accent);

        RectF{ 58, 20, 338, 338 }.draw(difficulty_theme.accent);
        if (draw_jacket) {
            JacketRegion(jacket).draw();
        }

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

        const double level_x = data.level < 10.0 ? 90.0 : 80.0;
        DrawDifficultyLevel(data.level, Vec2{ level_x, 503 }, 170.0 - level_x, difficulty_theme.text);
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

        FontAsset{ assets::font::Label }(U"HIGH").draw(17, 177, 470, difficulty_theme.accent);
        FontAsset{ assets::font::Label }(U"SCORE").draw(17, 177, 485, difficulty_theme.accent);
        FontAsset{ assets::font::Text }(data.high_score)
            .draw(40, Arg::bottomRight = Vec2{ 395, 520 }, difficulty_theme.text);

        if (data.high_score > 0 && !data.grade.isEmpty()) {
            drawBadge(Vec2{ 177, 446 }, Vec2{ 74, 23 }, data.grade, difficulty_theme.accent);
        }
        if (!data.clear_status.isEmpty()) {
            drawBadge(Vec2{ 255, 446 }, Vec2{ 140, 23 }, data.clear_status, difficulty_theme.accent);
        }

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

    void MusicCard::drawUnavailable() const {
        RectF{ CardSize }.draw(ColorF{ 0.92, 0.92, 0.95, 0.65 });

        constexpr double BandHeight = 58;
        const RectF band{ 0, (CardSize.y - BandHeight) / 2.0, CardSize.x, BandHeight };
        band.draw(ColorF{ 0.20, 0.20, 0.25, 0.88 });
        FontAsset{ assets::font::Label }(localization::GetText(localization::TextId::MusicCardChartUnavailable))
            .drawAt(25, band.center(), Palette::White);
    }

    void
    MusicCard::drawBadge(const Vec2& position, const Vec2& size, const StringView text, const ColorF& color) const {
        const ColorF translucent = color.withA(0.3);
        const Vec2 main_size{ size.x * 0.75, size.y };
        const Vec2 tail_size{ size.x * 0.25, size.y };
        RectF{ position, main_size }.draw(Arg::left = color, Arg::right = translucent);
        RectF{ position.movedBy(main_size.x, 0), tail_size }.draw(Arg::left = translucent, Arg::right = color);
        FontAsset{ assets::font::Label }(text)
            .draw(18, Arg::center = position.movedBy(size.x / 2.0, size.y / 2.0), Palette::White);
    }
}
