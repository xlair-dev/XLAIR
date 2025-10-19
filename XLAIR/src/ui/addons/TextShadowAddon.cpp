#include "TextShadowAddon.hpp"

namespace ui::addons {
    bool TextShadowAddon::init() {
        return true;
    }

    void TextShadowAddon::Draw(const StringView& text, const Font& font, double size, const Vec2& pos, const ColorF& color, const ColorF& shadow_color, const Vec2& shadow_offset) {
        if (auto p = Addon::GetAddon<TextShadowAddon>(TextShadowAddon::Name)) {
            p->draw(text, font, size, pos, color, shadow_color, shadow_offset);
        }
    }

    void TextShadowAddon::DrawAt(const StringView& text, const Font& font, double size, const Vec2& pos, const ColorF& color, const ColorF& shadow_color, const Vec2& shadow_offset) {
        if (auto p = Addon::GetAddon<TextShadowAddon>(TextShadowAddon::Name)) {
            p->drawAt(text, font, size, pos, color, shadow_color, shadow_offset);
        }
    }

    void TextShadowAddon::draw(const StringView& text, const Font& font, double size, const Vec2& pos, const ColorF& color, const ColorF& shadow_color, const Vec2& shadow_offset) const {
        {
            const ScopedRenderTarget2D target{ m_shadow_texture.clear(ColorF{ 1.0, 0.0 })};
            const ScopedRenderStates2D blend{ BlendState::MaxAlpha };
            const Transformer2D transform{ Mat3x2::Translate(shadow_offset) };
            font(text).draw(size, pos);
        }

        {
            Shader::Downsample(m_shadow_texture, m_gaussian_a4);
            Shader::GaussianBlur(m_gaussian_a4, m_gaussian_b4, m_gaussian_a4);
        }

        m_gaussian_a4.resized(app::consts::SceneSize).draw(ColorF{ shadow_color, 0.5 });

        font(text).draw(size, pos, color);
    }

    void TextShadowAddon::drawAt(const StringView& text, const Font& font, double size, const Vec2& pos, const ColorF& color, const ColorF& shadow_color, const Vec2& shadow_offset) const {
        {
            const ScopedRenderTarget2D target{ m_shadow_texture.clear(ColorF{ 1.0, 0.0 })};
            const ScopedRenderStates2D blend{ BlendState::MaxAlpha };
            const Transformer2D transform{ Mat3x2::Translate(shadow_offset) };
            font(text).drawAt(size, pos);
        }
        {
            Shader::Downsample(m_shadow_texture, m_gaussian_a4);
            Shader::GaussianBlur(m_gaussian_a4, m_gaussian_b4, m_gaussian_a4);
        }

        m_gaussian_a4.resized(app::consts::SceneSize).draw(shadow_color);

        font(text).drawAt(size, pos, color);
    }
}
