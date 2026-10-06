#pragma once

#include "ui/Design.hpp"
#include "ui/Scene.hpp"
#include "ui/components/GameScoreBar.hpp"
#include "ui/game/PlayfieldCamera.hpp"
#include "ui/game/PlayfieldRenderer3D.hpp"

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

        BasicCamera3D m_camera{ DesignSize,
                                game::PlayfieldCameraFOV,
                                game::PlayfieldCameraEye,
                                game::PlayfieldCameraFocus };
        game::PlayfieldRenderer3D m_playfield_renderer;
        mutable components::GameScoreBar m_score_bar;
        int64 m_current_sample = 0;
        double m_elapsed = 0.0;
        double m_note_speed = 1.0;
        bool m_playback_started = false;
        bool m_playback_finished = false;
    };
}
