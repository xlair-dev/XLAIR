#pragma once

#include "core/judgement/JudgeEngine.hpp"
#include "ui/Design.hpp"
#include "ui/Scene.hpp"
#include "ui/audio/GameplaySoundPlayer.hpp"
#include "ui/components/GameScoreBar.hpp"
#include "ui/game/LeadIn.hpp"
#include "ui/game/PlayfieldCamera.hpp"
#include "ui/game/PlayfieldRenderer3D.hpp"

#include <array>
#include <memory>
#include <span>

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
        enum class Phase {
            Intro,
            Reveal,
            LeadIn,
            Starting,
            Playing,
            Finished,
            Failed,
        };

        struct PressedInput {
            uint32 slider_pressed = 0;
            uint8 side_pressed = 0;
        };

        struct InputFrame {
            std::array<core::judgement::TouchEvent, app::controller::TouchZoneCount * 2> touches;
            std::array<core::judgement::SideEvent, app::controller::SideButtonCount> side_down;
            std::size_t touch_count = 0;
            std::size_t side_count = 0;
            PressedInput pressed;
        };

        void preparePlayback();
        void updatePlayback(const app::controller::Controller* controller, double delta_seconds);
        void readInput(const app::controller::Controller* controller, int64 judged_sample);
        [[nodiscard]]
        PressedInput pressedInputForDrawing() const;
        void processJudgements(std::span<const core::judgement::JudgementEvent> events);
        void finishPlayback();
        void drawReady() const;
        void drawIntro(double reveal) const;
        void drawFailure() const;
        void drawFadeOverlay(double opacity) const;

        BasicCamera3D m_camera{ DesignSize,
                                game::PlayfieldCameraFOV,
                                game::PlayfieldCameraEye,
                                game::PlayfieldCameraFocus };
        game::PlayfieldRenderer3D m_playfield_renderer;
        audio::GameplaySoundPlayer m_sound_player;
        InputFrame m_input;
        std::unique_ptr<core::judgement::JudgeEngine> m_judge_engine;
        mutable components::GameScoreBar m_score_bar;
        Optional<game::LeadIn> m_lead_in;
        Phase m_phase = Phase::Intro;
        double m_phase_elapsed = 0.0;
        String m_start_error;
        int64 m_current_sample = 0;
        int64 m_judgment_offset_samples = 0;
        Optional<core::judgement::JudgementEvent> m_last_judgement;
        double m_elapsed = 0.0;
        double m_judgement_display_seconds = 0.0;
        double m_note_speed = 3.0;
        bool m_autoplay = false;
    };
}
