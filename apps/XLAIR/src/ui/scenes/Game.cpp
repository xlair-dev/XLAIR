#include "Game.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/theme/Palette.hpp"

#include <SheetsAnalyzer/SliderHoldPath.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <span>

namespace xlair::ui::scenes {
    namespace {
        constexpr auto CancelButton = app::controller::MaintenanceButton::Button2;
        constexpr std::array SideButtonMapping{
            sheets::SideButton::LeftUpper,
            sheets::SideButton::LeftLower,
            sheets::SideButton::RightUpper,
            sheets::SideButton::RightLower,
        };

        [[nodiscard]]
        int64 JudgmentOffsetSamples(const double offset_seconds, const int64 sample_rate) {
            const long double samples = static_cast<long double>(offset_seconds) * sample_rate;
            constexpr long double limit = static_cast<long double>(std::numeric_limits<int64>::max() / 2);
            return static_cast<int64>(std::round(Clamp(samples, -limit, limit)));
        }

        [[nodiscard]]
        StringView JudgmentLabel(const core::judgement::Grade grade) {
            switch (grade) {
                case core::judgement::Grade::Perfect:
                    return U"PERFECT";
                case core::judgement::Grade::Great:
                    return U"GREAT";
                case core::judgement::Grade::Good:
                    return U"GOOD";
                case core::judgement::Grade::Miss:
                    return U"MISS";
            }
            return U"";
        }
    }

    Game::Game(const InitData& init) : SceneBase{ init } {
        getData().game_result.reset();
        if (getData().music_select_context) {
            getData().music_select_context->pause();
        }
        if (const auto& selection = getData().game_selection) {
            m_autoplay = selection->autoplay;
            getData().game_loader.start(*selection);
        }
        const double speed = getData().application->playSession().playOptions().note_speed;
        if (std::isfinite(speed) && speed > 0.0) {
            m_note_speed = Clamp(speed, 0.25, 10.0);
        }
    }

    Game::~Game() {
        m_sound_player.stop();
        getData().game_loader.cancel();
        if (const auto& selection = getData().game_selection) {
            getData().application->playSession().cancelPlay(selection->play_id);
        }
        getData().game_selection.reset();
    }

    void Game::update() {
        m_elapsed += Scene::DeltaTime();
        m_judgement_display_seconds += Scene::DeltaTime();
        const auto* controller = getData().application->controller();
        const auto& music = getData().game_loader.audio();
        const bool music_playing = m_playback_started && !m_playback_finished && music.isPlaying();
        if (music_playing) {
            m_current_sample = music.posSample();
        }
        readInput(controller, m_current_sample - m_judgment_offset_samples);
        const bool cancel = controller && controller->maintenanceButton(CancelButton).down();
        if (cancel || KeyEscape.pressedDuration() >= SecondsF{ 1.5 }) {
            m_sound_player.stop();
            if (music) {
                music.stop(SecondsF{ 0.25 });
            }
            getData().returning_from_game = true;
            changeScene(SceneState::MusicSelect, 500, CrossFade::No);
            return;
        }

        const auto& loader = getData().game_loader;
        if (loader.state() != game::GameLoader::State::Ready) {
            return;
        }

        if (!m_playback_started) {
            const auto& application = *getData().application;
            const double latency = application.config() ? application.config()->input.latency_offset_seconds : 0.0;
            const double judgment = application.playSession().playOptions().judgment_offset_ms / 1'000.0;
            m_judgment_offset_samples = JudgmentOffsetSamples(latency + judgment, loader.chart()->sample_rate);
            m_judge_engine = std::make_unique<core::judgement::JudgeEngine>(*loader.chart());
            m_sound_player.start(*loader.chart());
            music.play(audio::Music);
            m_playback_started = true;
            return;
        }

        if (!m_playback_finished) {
            if (music_playing) {
                // A positive offset compensates late input; it moves touch time earlier, not the visual chart.
                const int64 judged_sample = m_current_sample - m_judgment_offset_samples;
                if (m_autoplay) {
                    const auto events = m_judge_engine->advanceAuto(m_current_sample);
                    processJudgements(events);
                    return;
                }
                const auto events = m_judge_engine->advance(
                    judged_sample,
                    std::span{ m_input.touches.data(), m_input.touch_count },
                    std::span{ m_input.side_down.data(), m_input.side_count },
                    m_input.pressed.slider_pressed,
                    m_input.pressed.side_pressed
                );
                processJudgements(events);
            } else {
                finishPlayback();
            }
        }
    }

    void Game::readInput(const app::controller::Controller* controller, const int64 judged_sample) {
        m_input = {};
        if (!controller) {
            return;
        }
        for (std::size_t zone = 0; zone < app::controller::TouchZoneCount; ++zone) {
            const auto& state = controller->touchZone(zone);
            if (state.pressed()) {
                m_input.pressed.slider_pressed |= uint32{ 1 } << zone;
            }
            if (state.up()) {
                m_input.touches[m_input.touch_count++] = {
                    .zone = static_cast<uint8>(zone),
                    .sample = judged_sample,
                    .phase = core::judgement::TouchEvent::Phase::Up,
                };
            }
        }
        // Releases precede presses so a swipe across sensors in one frame can complete a FLICK.
        for (std::size_t zone = 0; zone < app::controller::TouchZoneCount; ++zone) {
            if (controller->touchZone(zone).down()) {
                m_input.touches[m_input.touch_count++] = { .zone = static_cast<uint8>(zone), .sample = judged_sample };
            }
        }
        for (std::size_t button = 0; button < app::controller::SideButtonCount; ++button) {
            const auto& state = controller->sideButton(static_cast<app::controller::SideButton>(button));
            if (state.pressed()) {
                m_input.pressed.side_pressed |= uint8{ 1 } << static_cast<uint8>(SideButtonMapping[button]);
            }
            if (state.down()) {
                m_input.side_down[m_input.side_count++] = { .button = SideButtonMapping[button],
                                                            .sample = judged_sample };
            }
        }
    }

    void Game::processJudgements(const std::span<const core::judgement::JudgementEvent> events) {
        m_sound_player.update(m_current_sample, *m_judge_engine, events, m_judge_engine->flickContacts());
        if (!events.empty()) {
            m_last_judgement = events.back();
            m_judgement_display_seconds = 0.0;
        }
    }

    void Game::finishPlayback() {
        auto& data = getData();
        m_current_sample = static_cast<int64>(data.game_loader.audio().samples());
        Array<core::judgement::JudgementEvent> events;
        if (m_autoplay) {
            events = m_judge_engine->advanceAuto(m_current_sample);
        }
        events.append(m_judge_engine->finish());
        processJudgements(events);
        m_sound_player.finish();
        m_playback_finished = true;

        if (data.game_selection) {
            const auto& selection = *data.game_selection;
            if (!data.application->playSession().completePlay(
                    selection.play_id,
                    selection.difficulty.id,
                    m_judge_engine->score(),
                    data.game_loader.chart()->total_combo
                )) {
                Logger << U"[Game] Could not complete the current session play.";
            }
            data.game_result = GameResult{
                .selection = selection,
                .score = m_judge_engine->score(),
                .total_combo = data.game_loader.chart()->total_combo,
            };
        }
        changeScene(SceneState::Result, 500, CrossFade::No);
    }

    void Game::draw() const {
        const auto& loader = getData().game_loader;
        if (loader.state() == game::GameLoader::State::Ready && loader.chart() && loader.projection()) {
            Scene::Rect().draw(theme::Palette::White);
            drawReady();
        } else if (loader.state() == game::GameLoader::State::Failed) {
            Scene::Rect().draw(theme::Palette::White);
            drawFailure();
        } else {
            Scene::Rect().draw(Palette::Black);
        }
    }

    Game::PressedInput Game::pressedInputForDrawing() const {
        auto pressed = m_input.pressed;
        const auto& chart = getData().game_loader.chart();
        if (!m_autoplay || !m_judge_engine || !chart) {
            return pressed;
        }

        // Autoplay sustains holds without physical input. These virtual presses
        // are used only for rendering, never fed back into judgement.
        for (std::size_t index = 0; index < chart->slider_holds.size(); ++index) {
            if (!m_judge_engine->sliderHoldActive(index)) {
                continue;
            }
            const auto lane = sheets::EvaluateSliderHoldLane(chart->slider_holds[index].points, m_current_sample);
            if (!lane) {
                continue;
            }
            const std::size_t end = Min<std::size_t>(
                (static_cast<std::size_t>(lane->start) + lane->width) * 2,
                app::controller::TouchZoneCount
            );
            for (std::size_t zone = static_cast<std::size_t>(lane->start) * 2; zone < end; ++zone) {
                pressed.slider_pressed |= uint32{ 1 } << zone;
            }
        }
        for (std::size_t index = 0; index < chart->side_holds.size(); ++index) {
            if (m_judge_engine->sideHoldActive(index)) {
                pressed.side_pressed |= uint8{ 1 } << static_cast<uint8>(chart->side_holds[index].button);
            }
        }
        return pressed;
    }

    void Game::drawReady() const {
        const auto& loader = getData().game_loader;
        const auto score = m_judge_engine ? m_judge_engine->score() : core::judgement::ScoreState{};
        const auto pressed = pressedInputForDrawing();
        m_playfield_renderer.draw(
            m_camera,
            *loader.chart(),
            *loader.projection(),
            m_current_sample,
            m_note_speed,
            score.combo,
            false,
            m_judge_engine.get(),
            pressed.slider_pressed,
            pressed.side_pressed
        );
        const auto& session = getData().application->playSession();
        if (const auto* user = session.user()) {
            const auto level = core::user::CalculateLevelProgress(user->xp);
            components::DrawUserNameplate(
                {
                    .display_name = user->display_name,
                    .rating = user->rating,
                    .level = level.level,
                    .level_progress = level.progress,
                },
                Point{ 59, 72 }
            );
        }
        if (const auto& selection = getData().game_selection) {
            components::DrawGameMusicPlate(
                {
                    .title = selection->title,
                    .jacket = getData().jackets.get(selection->music_id),
                    .difficulty_index = selection->difficulty.index,
                    .level = selection->difficulty.level,
                    .max_plays = selection->max_plays,
                    .remaining_plays = selection->remaining_plays,
                },
                Point{ 1480, 72 },
                m_elapsed
            );
        }
        m_score_bar.draw(
            { .score = score.score, .clear_gauge = score.clear_gauge },
            Point{ DesignSize.x / 2 - 434, 8 }
        );
        if (m_last_judgement && m_judgement_display_seconds < 0.75) {
            const Vec2 feedback_position{ DesignSize.x / 2.0, 780 };
            FontAsset{ assets::font::Display }(JudgmentLabel(m_last_judgement->grade))
                .drawAt(44, feedback_position, theme::Palette::White);
            if (m_last_judgement->grade != core::judgement::Grade::Miss) {
                const double milliseconds =
                    static_cast<double>(m_last_judgement->offset_samples) * 1'000.0 / loader.chart()->sample_rate;
                FontAsset{ assets::font::Text }(U"{:+.1f} ms"_fmt(milliseconds))
                    .drawAt(22, feedback_position.movedBy(0, 40), theme::Palette::White);
            }
        }
    }

    void Game::drawFailure() const {
        const Vec2 center{ DesignSize.x / 2.0, DesignSize.y / 2.0 };
        FontAsset{ assets::font::Text }(getData().game_loader.error()).drawAt(24, center, theme::Palette::Gray);
    }

    void Game::drawFadeIn(const double t) const {
        drawFadeOverlay(1.0 - Clamp(t, 0.0, 1.0));
    }

    void Game::drawFadeOut(const double t) const {
        drawFadeOverlay(Clamp(t, 0.0, 1.0));
    }

    void Game::drawFadeOverlay(const double opacity) const {
        draw();
        Scene::Rect().draw(ColorF{ 0.0, 0.0, 0.0, opacity });
    }
}
