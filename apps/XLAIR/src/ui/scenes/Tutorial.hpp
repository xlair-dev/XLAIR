#pragma once

#include "app/flows/Tutorial.hpp"
#include "ui/Scene.hpp"
#include "ui/audio/GameplaySoundPlayer.hpp"
#include "ui/components/TutorialSpotlight.hpp"
#include "ui/game/ControllerInput.hpp"
#include "ui/game/PlayfieldRenderer3D.hpp"
#include "ui/input/MenuInput.hpp"

namespace xlair::ui::scenes {
    class Tutorial final : public SceneBase {
    public:
        explicit Tutorial(const InitData& init);
        ~Tutorial() override;

        void update() override;
        void updateFadeIn(double t) override;
        void updateFadeOut(double t) override;
        void draw() const override;
        void drawFadeIn(double t) const override;
        void drawFadeOut(double t) const override;

        static void RegisterAssets();

    private:
        void updateMusicSample();
        void startPlayback();
        void updatePlayback(const app::controller::Controller* controller, double delta_seconds);
        void leave();
        void drawPrompt(double opacity, double exit_progress = 0.0) const;
        [[nodiscard]]
        double playbackPresentationSeconds() const;

        app::flows::Tutorial m_flow;
        input::MenuInput m_menu_input;
        game::ControllerInput m_input;
        BasicCamera3D m_camera;
        game::PlayfieldRenderer3D m_renderer;
        components::TutorialSpotlight m_spotlight;
        audio::GameplaySoundPlayer m_sounds;
        Audio m_music;
        sheets::Chart m_chart;
        Optional<playfield::ChartProjection> m_projection;
        std::unique_ptr<core::judgement::JudgeEngine> m_judge_engine;
        int64 m_music_sample = 0;
        int64 m_last_audio_sample = 0;
        int64 m_loop_offset = 0;
        int64 m_judgment_offset_samples = 0;
        double m_state_elapsed = 0.0;
        double m_audio_start_elapsed = 0.0;
        String m_error;
        bool m_audio_started = false;
        bool m_leaving = false;
    };
}
