#pragma once

#include "ui/Scene.hpp"
#include "ui/components/CardCarousel.hpp"
#include "ui/components/SettingCard.hpp"
#include "ui/components/SliderMappingGuide.hpp"
#include "ui/transitions/CardTransition.hpp"

namespace xlair::ui::scenes {
    class Settings final : public SceneBase {
    public:
        explicit Settings(const InitData& init);

        void update() override;
        void updateFadeIn(double t) override;
        void updateFadeOut(double t) override;
        void draw() const override;
        void drawFadeIn(double t) const override;
        void drawFadeOut(double t) const override;

        static void RegisterAssets();

    private:
        enum class SettingItem {
            Speed,
            JudgmentOffset,
            Mirror,
        };

        void handleInput();
        void updatePresentation();
        bool moveItem(int32 direction);
        bool adjustValue(int32 direction);
        void drawScene(const transitions::CardTransition& transition) const;
        void drawCards(const transitions::CardTransition& transition) const;
        void drawCard(std::size_t index, const RectF& region, const transitions::CardTransition& transition) const;

        struct Assets {
            static constexpr AssetNameView Header{ U"XLAIR.Settings.Header" };
        };

        components::CardCarousel m_card_carousel;
        mutable components::SettingCard m_setting_card;
        Array<components::SettingCardData> m_setting_cards;
        Array<components::SliderMapping> m_slider_mappings;
        std::size_t m_selected_index = 0;

        // Temporary scene-local values; player-option persistence is not connected yet.
        int32 m_note_speed_steps = 4; // One step is 0.25x.
        int32 m_judgment_offset_ms = 0;
        bool m_mirror = false;
    };
}
