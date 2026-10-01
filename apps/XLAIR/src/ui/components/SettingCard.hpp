#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    struct SettingCardData {
        String title;
        String description;
        String value;
        bool can_decrease = true;
        bool can_increase = true;
        bool show_arrow_labels = true;
    };

    class SettingCard {
    public:
        SettingCard();

        [[nodiscard]]
        const MSRenderTexture& render(const SettingCardData& data);

        [[nodiscard]]
        static constexpr Size size() noexcept {
            return CardSize;
        }

    private:
        static constexpr Size CardSize{ 416, 545 };

        MSRenderTexture m_render_texture;
    };
}
