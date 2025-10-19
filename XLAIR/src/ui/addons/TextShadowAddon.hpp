#pragma once
#include "Common.hpp"
#include "app/consts/Scene.hpp"

namespace ui::addons {
    class TextShadowAddon : public IAddon {
    public:
        inline static constexpr StringView Name{ U"TextShadowAddon" };

        static void Draw(const StringView& text, const Font& font, double size, const Vec2& pos, const ColorF& color, const ColorF& shadow_color, const Vec2& shadow_offset);
        static void DrawAt(const StringView& text, const Font& font, double size, const Vec2& pos, const ColorF& color, const ColorF& shadow_color, const Vec2& shadow_offset);

    private:
        bool init() override;
        void draw(const StringView& text, const Font& font, double size, const Vec2& pos, const ColorF& color, const ColorF& shadow_color, const Vec2& shadow_offset) const;
        void drawAt(const StringView& text, const Font& font, double size, const Vec2& pos, const ColorF& color, const ColorF& shadow_color, const Vec2& shadow_offset) const;

        RenderTexture m_shadow_texture{ app::consts::SceneSize };
        RenderTexture m_gaussian_a4{ app::consts::SceneSize / 4 };
        RenderTexture m_gaussian_b4{ app::consts::SceneSize / 4 };
    };
}
