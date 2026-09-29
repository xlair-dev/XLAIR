#pragma once

#include "ui/Scene.hpp"
#include "ui/components/MusicCard.hpp"
#include "ui/components/SliderMappingGuide.hpp"

namespace xlair::ui::scenes {
    class MusicSelect final : public SceneBase {
    public:
        explicit MusicSelect(const InitData& init);

        void update() override;
        void draw() const override;

        static void RegisterAssets();

    private:
        struct Assets {
            static constexpr AssetNameView Header{ U"XLAIR.MusicSelect.Header" };
        };

        void handleInput();
        void updateAnimation();
        void drawCards() const;
        void drawCard(std::size_t music_index, const RectF& region, double text_elapsed) const;
        void drawArrows() const;
        void drawEmptyCatalog() const;

        mutable components::MusicCard m_music_card;
        Array<components::SliderMapping> m_slider_mappings;
        double m_scroll_offset = 0.0;
        double m_scroll_velocity = 0.0;
        double m_text_elapsed = 0.0;
    };
}
