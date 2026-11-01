#pragma once

#include "ui/Scene.hpp"
#include "ui/components/CardCarousel.hpp"
#include "ui/components/MusicCard.hpp"
#include "ui/components/SliderMappingGuide.hpp"
#include "ui/transitions/CardTransition.hpp"
#include "ui/input/MenuInput.hpp"

namespace xlair::ui::scenes {
    class MusicSelect final : public SceneBase {
    public:
        explicit MusicSelect(const InitData& init);

        void update() override;
        void updateFadeIn(double t) override;
        void updateFadeOut(double t) override;
        void draw() const override;
        void drawFadeIn(double t) const override;
        void drawFadeOut(double t) const override;

        static void RegisterAssets();

    private:
        struct Assets {
            static constexpr AssetNameView Header{ U"XLAIR.MusicSelect.Header" };
        };

        void handleInput();
        bool startSelectedMusic();
        void updatePresentation();
        void drawGameTransitionBackground() const;
        void drawScene(const transitions::CardTransition& transition) const;
        void drawCards(const transitions::CardTransition& transition) const;
        void drawCard(
            std::size_t music_index,
            const RectF& region,
            double text_elapsed,
            const transitions::CardTransition& transition
        ) const;
        void drawEmptyCatalog() const;

        components::CardCarousel m_card_carousel;
        input::MenuInput m_menu_input;
        mutable components::MusicCard m_music_card;
        mutable MSRenderTexture m_game_transition_background;
        Array<components::SliderMapping> m_slider_mappings;
        double m_text_elapsed = 0.0;
        double m_active_elapsed = 0.0;
        bool m_auto_confirm_attempted = false;
        bool m_fade_to_game = false;
        bool m_fade_from_game = false;
    };
}
