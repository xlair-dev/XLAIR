#include "GameScoreBar.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::components {
    void GameScoreBar::draw(const GameScoreBarData& data, const Point& position, const int32 width) {
        const ScopedViewport2D viewport{ position, width, 150 };
        const Font font = FontAsset{ assets::font::Display };
        const String score_text = U"{:0>7}"_fmt(data.score);

        font(U"SCORE").drawBase(32, 32, 42, theme::Palette::Gray);

        if (m_cached_score != data.score || m_cached_width != width) {
            updateScoreShadow(score_text, font, width);
            m_cached_score = data.score;
            m_cached_width = width;
        }
        m_gaussian_a4.resized(DesignSize).draw(theme::Palette::DarkBlue);
        font(score_text).drawAt(48, Vec2{ width / 2.0, 30 }, theme::Palette::White);

        TextureAsset{ assets::texture::GameScoreBarFrame }.resized(width, 67).draw(0, 49);

        const double gauge = Clamp(data.clear_gauge, 0.0, 1.0);
        const Vec2 gauge_center{ 40.0 + (width - 80.0) * gauge, 49 + 67 / 2.0 };
        const ColorF color = gauge >= 0.7 ? theme::Palette::Pink : theme::Palette::Cyan;

        Line{ 40.0, gauge_center.y, gauge_center.x, gauge_center.y }
            .draw(LineStyle::RoundCap, 6.0, color, color.withA(0.53));

        const Quad diamond{ gauge_center.movedBy(0, -35),
                            gauge_center.movedBy(35, 0),
                            gauge_center.movedBy(0, 35),
                            gauge_center.movedBy(-35, 0) };
        diamond.draw(color.withA(0.6));
        diamond.scaledAt(gauge_center, 0.5).draw(theme::Palette::White);
    }

    void GameScoreBar::updateScoreShadow(const StringView score_text, const Font& font, const int32 width) {
        {
            const ScopedRenderTarget2D target{ m_shadow_texture.clear(ColorF{ 1.0, 0.0 }) };
            const ScopedRenderStates2D blend{ BlendState::MaxAlpha };
            const Transformer2D transform{ Mat3x2::Translate(Vec2{ 1, 1 }) };
            font(score_text).drawAt(48, Vec2{ width / 2.0, 30 });
        }
        Shader::Downsample(m_shadow_texture, m_gaussian_a4);
        Shader::GaussianBlur(m_gaussian_a4, m_gaussian_b4, m_gaussian_a4);
    }
}
