#pragma once

#include "ui/Scene.hpp"
#include "ui/components/CardCarousel.hpp"
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
        void drawCards() const;
        void drawCard(std::size_t music_index, const RectF& region, double text_elapsed) const;
        void drawEmptyCatalog() const;

        components::CardCarousel m_card_carousel;
        mutable components::MusicCard m_music_card;
        Array<components::SliderMapping> m_slider_mappings;
        double m_text_elapsed = 0.0;
    };
}
