#include "SettingCard.hpp"

#include "ui/assets/Assets.hpp"

#include "ui/theme/Palette.hpp"

namespace xlair::ui::components {
    namespace {
        constexpr SizeF ValueArrowSize{ 76, 57 };

        void DrawValueArrow(const Vec2& center, const bool point_right, const bool enabled, const bool show_label) {
            const auto arrow = TextureAsset{ enabled ? assets::texture::SettingValueArrowEnabled
                                                     : assets::texture::SettingValueArrowDisabled }
                                   .resized(ValueArrowSize);
            if (point_right) {
                arrow.mirrored().drawAt(center);
            } else {
                arrow.drawAt(center);
            }
            if (show_label) {
                FontAsset{ assets::font::Text }(point_right ? U"+" : U"-").drawAt(32, center.movedBy(0, -3));
            }
        }
    }

    SettingCard::SettingCard() : m_render_texture{ CardSize } {}

    const MSRenderTexture& SettingCard::render(const SettingCardData& data) {
        m_render_texture.clear(theme::Palette::Gray);
        {
            const ScopedRenderTarget2D target{ m_render_texture };

            const double center_x = CardSize.x / 2.0;
            const double center_y = CardSize.y / 2.0;

            {
                constexpr double GradientStart = 0.65;
                const double radius = center_x * 1.7;
                const double inner_radius = radius * GradientStart;
                const Vec2 center{ center_x, center_y };
                const Transformer2D transform{ Mat3x2::Scale(1.0, center_y / center_x, Float2{ center }) };

                Circle{ center, inner_radius }.draw(Palette::White);
                Circle{ center, inner_radius }
                    .drawFrame(0, radius - inner_radius, Palette::White, theme::Palette::Gray);
            }

            RectF{ CardSize }.drawFrame(2, 0, theme::Palette::Pink);

            FontAsset{ assets::font::Text }(data.title).drawBaseAt(36, Vec2{ center_x, 144 }, theme::Palette::BlueGray);
            FontAsset{ assets::font::Text }(data.value).drawBaseAt(80, Vec2{ center_x, 280 }, theme::Palette::BlueGray);

            DrawValueArrow(Vec2{ 75, 253 }, false, data.can_decrease, data.show_arrow_labels);
            DrawValueArrow(Vec2{ CardSize.x - 75, 253 }, true, data.can_increase, data.show_arrow_labels);

            Line{ 28, 165, 390, 165 }.draw(2, theme::Palette::Pink);

            constexpr auto text_region = RectF{ 38, 335, 343, 80 };

            FontAsset{ assets::font::Text }(data.description).draw(18, text_region, theme::Palette::BlueGray);
        }
        Graphics2D::Flush();
        m_render_texture.resolve();
        return m_render_texture;
    }
}
