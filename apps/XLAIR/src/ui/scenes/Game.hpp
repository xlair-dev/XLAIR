#pragma once

#include "ui/Scene.hpp"
#include "ui/components/GameScoreBar.hpp"
#include "ui/game/PlayfieldRenderer.hpp"

namespace xlair::ui::scenes {
    class Game final : public SceneBase {
    public:
        explicit Game(const InitData& init);
        ~Game() override;

        void update() override;
        void draw() const override;
        void drawFadeIn(double t) const override;
        void drawFadeOut(double t) const override;

    private:
        void drawReady() const;
        void drawFailure() const;
        void drawFadeOverlay(double opacity) const;

        game::PlayfieldRenderer m_playfield_renderer;
        mutable components::GameScoreBar m_score_bar;
        int64 m_current_sample = 0;
        double m_elapsed = 0.0;
        double m_pixels_per_second = 360.0;
        bool m_playback_started = false;
        bool m_playback_finished = false;
    };
}
