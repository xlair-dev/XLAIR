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
        enum class SettingItem {
            Speed,
            JudgmentOffset,
            Mirror,
        };

        void handleInput();
        bool moveItem(int32 direction);
        bool adjustValue(int32 direction);
        void drawCards() const;
        void drawCard(std::size_t index, const RectF& region) const;
        void drawArrows() const;

        struct Assets {
            static constexpr AssetNameView Header{ U"XLAIR.Settings.Header" };
        };

        mutable components::SettingCard m_setting_card;
        Array<components::SettingCardData> m_setting_cards;
        Array<components::SliderMapping> m_slider_mappings;
        std::size_t m_selected_index = 0;
        double m_scroll_offset = 0.0;
        double m_scroll_velocity = 0.0;

        // Temporary scene-local values; player-option persistence is not connected yet.
        int32 m_note_speed_steps = 10;
        int32 m_judgment_offset_ms = 0;
        bool m_mirror = false;
    };
}
