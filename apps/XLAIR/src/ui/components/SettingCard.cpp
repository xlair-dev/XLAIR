#include "SettingCard.hpp"

#include "ui/assets/Assets.hpp"

namespace xlair::ui::components {
    SettingCard::SettingCard() : m_render_texture{ CardSize } {}

    const MSRenderTexture& SettingCard::render(const SettingCardData& data, const ColorF& background) {
        m_render_texture.clear(background);
        {
            const ScopedRenderTarget2D target{ m_render_texture };
            const double center_x = CardSize.x / 2.0;
            FontAsset{ assets::font::Label }(data.title).drawAt(28, Vec2{ center_x, 80 }, Palette::White);
            FontAsset{ assets::font::Text }(data.value).drawAt(72, Vec2{ center_x, 240 }, Palette::White);
            FontAsset{ assets::font::Text }(data.unit).drawAt(28, Vec2{ center_x, 310 }, Palette::White);
            FontAsset{ assets::font::Text }(data.description).drawAt(22, Vec2{ center_x, 435 }, Palette::White);
        }
        Graphics2D::Flush();
        m_render_texture.resolve();
        return m_render_texture;
    }
}
