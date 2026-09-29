#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    struct SettingCardData {
        String title;
        String description;
        String value;
        String unit;
    };

    class SettingCard {
    public:
        SettingCard();

        [[nodiscard]]
        const MSRenderTexture& render(const SettingCardData& data, const ColorF& background);

        [[nodiscard]]
        static constexpr Size size() noexcept {
            return CardSize;
        }

    private:
        static constexpr Size CardSize{ 416, 545 };

        MSRenderTexture m_render_texture;
    };
}
