#pragma once

#include "ui/Scene.hpp"
#include "ui/components/SettingCard.hpp"
#include "ui/components/SliderMappingGuide.hpp"

namespace xlair::ui::scenes {
    class Settings final : public SceneBase {
    public:
        explicit Settings(const InitData& init);

        void update() override;
        void draw() const override;

        static void RegisterAssets();

    private:
        void drawCards() const;

        struct Assets {
            static constexpr AssetNameView Header{ U"XLAIR.Settings.Header" };
        };

        mutable components::SettingCard m_setting_card;
        Array<components::SettingCardData> m_setting_cards;
        Array<components::SliderMapping> m_slider_mappings;
    };
}
