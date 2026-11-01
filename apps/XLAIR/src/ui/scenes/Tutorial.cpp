#include "Tutorial.hpp"

#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/SliderMappingGuide.hpp"
#include "ui/components/TutorialPanel.hpp"
#include "ui/game/JudgementClock.hpp"
#include "ui/game/PlayfieldCamera.hpp"
#include "ui/game/TutorialFocus.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/presentation/TutorialStart.hpp"
#include "ui/theme/Palette.hpp"
#include "ui/transitions/CardTransition.hpp"
#include "ui/transitions/ScreenWipe.hpp"

namespace xlair::ui::scenes {
    namespace {
        using State = app::flows::Tutorial::State;
        using Lesson = app::flows::Tutorial::Lesson;
        using localization::GetText;
        using localization::TextId;
        constexpr double InputWaitSeconds = 0.25;
        constexpr double CompleteWaitSeconds = 4.0;
        constexpr double AudioStartTimeout = 2.0;
        constexpr double NoteSpeed = 2.0;
        constexpr std::array MenuKeys{ KeyEnter, KeyEscape };

        [[nodiscard]]
        TextId LessonTitle(const Lesson lesson) {
            switch (lesson) {
                case Lesson::Tap:
                    return TextId::TutorialTapTitle;
                case Lesson::Flick:
                    return TextId::TutorialFlickTitle;
                case Lesson::Hold:
                    return TextId::TutorialHoldTitle;
                case Lesson::SideUpper:
                    return TextId::TutorialSideUpperTitle;
                case Lesson::SideLower:
                    return TextId::TutorialSideLowerTitle;
            }
            return TextId::TutorialTapTitle;
        }

        [[nodiscard]]
        TextId LessonExplanation(const Lesson lesson) {
            switch (lesson) {
                case Lesson::Tap:
                    return TextId::TutorialExplanation;
                case Lesson::Flick:
                    return TextId::TutorialFlickExplanation;
                case Lesson::Hold:
                    return TextId::TutorialHoldExplanation;
                case Lesson::SideUpper:
                    return TextId::TutorialSideUpperExplanation;
                case Lesson::SideLower:
                    return TextId::TutorialSideLowerExplanation;
            }
            return TextId::TutorialExplanation;
        }

        [[nodiscard]]
        double GuideOpacity(const app::flows::Tutorial::Guide& guide, const int64 sample, const int64 sample_rate) {
            const auto boundary = [sample_rate](const double beat) {
                return std::round(static_cast<long double>(beat) * sample_rate * 60.0L / app::flows::Tutorial::BPM);
            };
            const double elapsed = static_cast<double>((sample - boundary(guide.begin_beat)) / sample_rate);
            const double remaining = static_cast<double>((boundary(guide.end_beat) - sample) / sample_rate);
            return EaseOutCubic(Clamp(Min(elapsed / 0.22, remaining / 0.18), 0.0, 1.0));
        }
    }

    Tutorial::Tutorial(const InitData& init)
        : SceneBase{ init },
          m_camera{ DesignSize, game::PlayfieldCameraFOV, game::PlayfieldCameraEye, game::PlayfieldCameraFocus },
          m_music{ AudioAsset{ assets::bgm::Login } } {
        if (const auto& context = getData().music_select_context) {
            context->pause();
        }
        m_music.stop();
        const auto sample_rate = m_music.sampleRate();
        if (sample_rate == 0 || m_music.samples() == 0) {
            m_error = GetText(TextId::TutorialUnavailable);
            return;
        }
        auto chart = sheets::LoadChart(Resource(U"ui/tutorial/tutorial.sus"), { .sample_rate = sample_rate });
        if (!chart || chart->slider_notes.isEmpty() || chart->slider_holds.isEmpty() || chart->side_notes.isEmpty()) {
            m_error = GetText(TextId::TutorialUnavailable);
            for (const auto& diagnostic : chart.diagnostics) {
                Logger << U"[Tutorial] " + diagnostic.message;
            }
            return;
        }
        m_chart = std::move(*chart);
        // Keep the field's measure lines on the same stop timeline as its notes.
        m_chart.main_timeline = m_chart.slider_notes.front().timeline;
        m_projection.emplace(m_chart);
        const auto& application = *getData().application;
        const double latency = application.config() ? application.config()->input.latency_offset_seconds : 0.0;
        const double judgment = application.playSession().playOptions().judgment_offset_ms / 1'000.0;
        m_judgment_offset_samples = game::JudgmentOffsetSamples(latency + judgment, sample_rate);
    }

    Tutorial::~Tutorial() {
        m_sounds.stop();
        m_music.stop();
    }

    void Tutorial::updateMusicSample() {
        const int64 position = Max<int64>(0, m_music.posSample());
        if (position + m_music.sampleRate() / 2 < m_last_audio_sample) {
            m_loop_offset += static_cast<int64>(m_music.samples());
        }
        m_last_audio_sample = position;
        m_music_sample = Max(m_music_sample, m_loop_offset + position);
    }

    void Tutorial::update() {
        getData().login_to_tutorial_wipe = false;
        if (m_leaving) {
            return;
        }
        const double delta_seconds = Scene::DeltaTime();
        m_state_elapsed += delta_seconds;
        const auto* controller = getData().application->controller();
        const bool entering_playfield =
            m_flow.state() == State::Playing && presentation::MakeTutorialStart(playbackPresentationSeconds()).active;
        m_menu_input
            .update(controller, MenuKeys, delta_seconds, m_state_elapsed >= InputWaitSeconds && !entering_playfield);
        if ((controller && controller->maintenanceButton(app::controller::MaintenanceButton::Button2).down()) ||
            m_menu_input.keyDown(KeyEscape)) {
            leave();
            return;
        }
        if (!m_error.isEmpty()) {
            if (m_menu_input.keyDown(KeyEnter) || m_menu_input.touchDown(0, 16)) {
                leave();
            }
            return;
        }
        const auto previous = m_flow.state();
        switch (previous) {
            case State::Prompt:
                if (m_menu_input.touchDown(0, 8) || m_menu_input.keyDown(KeyEnter)) {
                    m_flow.choose(true);
                    startPlayback();
                } else if (m_menu_input.touchDown(8, 8)) {
                    m_flow.choose(false);
                    leave();
                    return;
                }
                break;
            case State::Playing:
                updatePlayback(controller, delta_seconds);
                break;
            case State::Complete:
                if (m_state_elapsed >= CompleteWaitSeconds || m_menu_input.keyDown(KeyEnter) ||
                    m_menu_input.touchDown(0, 16)) {
                    leave();
                    return;
                }
                break;
            case State::Skipped:
                leave();
                return;
        }
        if (m_flow.state() != previous) {
            m_state_elapsed = 0.0;
            m_menu_input = {};
        }
    }

    void Tutorial::startPlayback() {
        m_judge_engine = std::make_unique<core::judgement::JudgeEngine>(m_chart);
        m_sounds.start(m_chart);
        m_music.seekSamples(0);
        m_music.setLoop(true);
        m_music.play(audio::Music);
    }

    void Tutorial::updatePlayback(const app::controller::Controller* controller, const double delta_seconds) {
        if (!m_audio_started) {
            m_audio_start_elapsed += delta_seconds;
            if (m_music.isPlaying() || m_music.posSample() > 0) {
                m_audio_started = true;
            } else if (m_audio_start_elapsed >= AudioStartTimeout) {
                m_error = GetText(TextId::TutorialUnavailable);
                m_sounds.stop();
                m_music.stop();
                return;
            } else {
                return;
            }
        }
        updateMusicSample();
        const int64 judged_sample = game::JudgedSample(m_music_sample, m_judgment_offset_samples);
        m_input.read(controller, judged_sample);
        const auto events = m_judge_engine->advance(
            judged_sample,
            std::span{ m_input.touches.data(), m_input.touch_count },
            std::span{ m_input.side_down.data(), m_input.side_count },
            m_input.pressed.slider_pressed,
            m_input.pressed.side_pressed
        );
        m_sounds.update(m_music_sample, *m_judge_engine, events, m_judge_engine->flickContacts());
        m_renderer.addJudgements(m_chart, events);
        m_flow.advance(m_music_sample, m_chart.sample_rate);
        if (m_flow.state() == State::Complete) {
            m_sounds.finish();
        }
    }

    void Tutorial::leave() {
        if (!changeScene(SceneState::MusicSelect, transitions::CardTransitionMillisec, CrossFade::No)) {
            return;
        }
        m_leaving = true;
        m_sounds.stop();
        m_music.stop(Duration{ 0.3 });
        audio::PlaySoundEffect(audio::SoundEffect::Next);
    }

    double Tutorial::playbackPresentationSeconds() const {
        return Max(m_state_elapsed, static_cast<double>(m_music_sample) / m_chart.sample_rate);
    }

    void Tutorial::drawPrompt(const double opacity, const double exit_progress) const {
        if (opacity <= 0.0) {
            return;
        }
        const Vec2 center{ DesignSize.x / 2.0, DesignSize.y / 2.0 };
        const Transformer2D transform{ Mat3x2::Scale(1.0 - 0.025 * exit_progress, center) *
                                       Mat3x2::Translate(0.0, -22.0 * exit_progress) };
        components::DrawTutorialPanel(
            { .kind = components::TutorialPanelKind::Prompt, .title = GetText(TextId::TutorialPrompt) },
            opacity
        );
        const ScopedColorMul2D alpha{ ColorF{ 1.0, opacity } };
        components::DrawSliderMappingGuide(
            {
                { .region = { .start = 0, .width = 8 },
                  .label = GetText(TextId::TutorialYes),
                  .color = theme::Palette::Pink },
                { .region = { .start = 8, .width = 8 },
                  .label = GetText(TextId::TutorialNo),
                  .color = theme::Palette::Cyan },
            },
            RectF{ 210, 1010, 1500, 70 }
        );
    }

    void Tutorial::draw() const {
        Scene::Rect().draw(theme::Palette::White);
        components::TutorialPanelData panel;
        double opacity = EaseOutCubic(Clamp(m_state_elapsed / 0.2, 0.0, 1.0));
        if (!m_error.isEmpty()) {
            panel.kind = components::TutorialPanelKind::Complete;
            panel.title = m_error;
        } else {
            switch (m_flow.state()) {
                case State::Prompt:
                    drawPrompt(opacity);
                    return;
                case State::Playing: {
                    m_renderer.draw(
                        m_camera,
                        m_chart,
                        *m_projection,
                        m_music_sample,
                        NoteSpeed,
                        0,
                        false,
                        m_judge_engine.get(),
                        m_input.pressed.slider_pressed,
                        m_input.pressed.side_pressed
                    );
                    const auto guide = m_flow.guide();
                    if (guide) {
                        opacity = GuideOpacity(*guide, m_music_sample, m_chart.sample_rate);
                        const auto regions = game::TutorialFocusRegions(
                            m_camera,
                            m_chart,
                            *m_projection,
                            *guide,
                            m_music_sample,
                            NoteSpeed
                        );
                        m_spotlight.draw(regions, opacity);
                    }
                    components::DrawGameMusicPlate(
                        components::GameMusicPlateLabelData{
                            .heading = U"TUTORIAL",
                            .title = GetText(LessonTitle(m_flow.lesson())),
                        },
                        Point{ 1480, 72 },
                        m_state_elapsed
                    );
                    const auto start = presentation::MakeTutorialStart(playbackPresentationSeconds());
                    if (start.active) {
                        Scene::Rect().draw(ColorF{ 1.0, 1.0 - start.field_opacity });
                        drawPrompt(start.prompt_opacity, start.prompt_exit);
                    }
                    if (!guide) {
                        return;
                    }
                    panel.kind = components::TutorialPanelKind::Explain;
                    panel.title = GetText(LessonTitle(m_flow.lesson()));
                    panel.message = GetText(LessonExplanation(m_flow.lesson()));
                    break;
                }
                case State::Complete:
                    panel.kind = components::TutorialPanelKind::Complete;
                    panel.title = GetText(TextId::TutorialComplete);
                    panel.message = GetText(TextId::TutorialCompleteMessage);
                    break;
                case State::Skipped:
                    drawPrompt(1.0);
                    return;
            }
        }
        components::DrawTutorialPanel(panel, opacity);
        if (!m_error.isEmpty() || m_flow.state() == State::Complete) {
            components::DrawSliderMappingGuide(
                { { .region = { .start = 0, .width = 16 },
                    .label = GetText(TextId::TutorialContinue),
                    .color = theme::Palette::Cyan } },
                RectF{ 210, 1010, 1500, 70 }
            );
        }
    }

    void Tutorial::updateFadeIn(const double) {
        m_state_elapsed += Scene::DeltaTime();
        m_menu_input.update(getData().application->controller(), MenuKeys, Scene::DeltaTime(), false);
    }

    void Tutorial::updateFadeOut(const double) {}

    void Tutorial::drawFadeIn(const double t) const {
        draw();
        if (getData().login_to_tutorial_wipe) {
            transitions::DrawScreenWipe(0.5 + Clamp(t, 0.0, 1.0) * 0.5);
        } else {
            Scene::Rect().draw(ColorF{ 1.0, 1.0 - Clamp(t, 0.0, 1.0) });
        }
    }

    void Tutorial::drawFadeOut(const double t) const {
        draw();
        Scene::Rect().draw(ColorF{ 1.0, Clamp(t, 0.0, 1.0) });
    }

    void Tutorial::RegisterAssets() {
        String latin = U"TUTORIAL";
        String cjk;
        for (const auto id : {
                 TextId::TutorialPrompt,
                 TextId::TutorialYes,
                 TextId::TutorialNo,
                 TextId::TutorialTapTitle,
                 TextId::TutorialExplanation,
                 TextId::TutorialFlickTitle,
                 TextId::TutorialFlickExplanation,
                 TextId::TutorialHoldTitle,
                 TextId::TutorialHoldExplanation,
                 TextId::TutorialSideUpperTitle,
                 TextId::TutorialSideUpperExplanation,
                 TextId::TutorialSideLowerTitle,
                 TextId::TutorialSideLowerExplanation,
                 TextId::TutorialComplete,
                 TextId::TutorialCompleteMessage,
                 TextId::TutorialContinue,
                 TextId::TutorialUnavailable,
             }) {
            for (const char32 ch : GetText(id)) {
                if (ch >= U' ') {
                    (ch < 0x80 ? latin : cjk).push_back(ch);
                }
            }
        }
        if (!FontAsset{ assets::font::Text }.preload(latin.sorted_and_uniqued()) ||
            !FontAsset{ assets::font::CjkFallback }.preload(cjk.sorted_and_uniqued())) {
            throw Error{ U"Failed to preload the Tutorial glyphs." };
        }
    }
}
