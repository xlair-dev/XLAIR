#include "Game.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/GameStart.hpp"
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
        constexpr double JudgementDisplayDuration = 0.75;
        constexpr double JudgementPopDuration = 0.10;
        constexpr double JudgementFadeDuration = 0.20;
        constexpr double JudgementPopScale = 0.05;
        constexpr double AudioStartTimeout = 2.0;
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
        int64 JudgedSample(const int64 sample, const int64 offset) {
            const long double adjusted = static_cast<long double>(sample) - offset;
            constexpr auto Minimum = static_cast<long double>(std::numeric_limits<int64>::min());
            constexpr auto Maximum = static_cast<long double>(std::numeric_limits<int64>::max());
            if (adjusted <= Minimum) {
                return std::numeric_limits<int64>::min();
            }
            if (adjusted >= Maximum) {
                return std::numeric_limits<int64>::max();
            }
            return static_cast<int64>(adjusted);
        }

        [[nodiscard]]
        AssetNameView JudgmentTexture(const core::judgement::Grade grade) {
            switch (grade) {
                case core::judgement::Grade::Perfect:
                    return assets::texture::ResultPerfect;
                case core::judgement::Grade::Great:
                    return assets::texture::ResultGreat;
                case core::judgement::Grade::Good:
                    return assets::texture::ResultGood;
                case core::judgement::Grade::Miss:
                    return assets::texture::ResultMiss;
            }
            return assets::texture::ResultMiss;
        }
    }

    Game::Game(const InitData& init) : SceneBase{ init } {
        getData().game_result.reset();
        if (getData().music_select_context) {
            getData().music_select_context->pause();
        }
        if (const auto& selection = getData().game_selection) {
            m_autoplay = selection->autoplay;
            if (const auto& transition = getData().music_start) {
                m_start_jacket = transition->jacket;
                // The incoming scene transition already plays the intro's entrance.
                m_elapsed = components::GameStartEnterDuration;
            } else {
                m_start_jacket = getData().jackets.get(selection->music_id);
            }
            getData().game_loader.start(*selection);
        }
        const double speed = getData().application->playSession().playOptions().note_speed;
        if (std::isfinite(speed) && speed > 0.0) {
            m_note_speed = Clamp(speed, 0.25, 10.0);
        }
    }

    Game::~Game() {
        getData().music_start.reset();
        m_sound_player.stop();
        getData().game_loader.cancel();
        if (const auto& selection = getData().game_selection) {
            getData().application->playSession().cancelPlay(selection->play_id);
        }
        getData().game_selection.reset();
    }

    void Game::update() {
        getData().music_start.reset();
        const double delta_seconds = Scene::DeltaTime();
        m_elapsed += delta_seconds;
        m_phase_elapsed += delta_seconds;
        m_judgement_display_seconds += delta_seconds;
        const auto* controller = getData().application->controller();
        const auto& music = getData().game_loader.audio();
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

        if (m_phase == Phase::Intro) {
            if (m_phase_elapsed >= components::GameStartMinimumDuration) {
                preparePlayback();
            }
            return;
        }
        if (m_phase == Phase::Reveal) {
            if (m_phase_elapsed >= components::GameStartRevealDuration) {
                m_phase = Phase::LeadIn;
                m_phase_elapsed = 0.0;
                m_input = {};
            }
            return;
        }
        if (m_phase != Phase::Finished && m_phase != Phase::Failed) {
            updatePlayback(controller, delta_seconds);
        }
    }

    void Game::preparePlayback() {
        const auto& application = *getData().application;
        const auto& chart = *getData().game_loader.chart();
        const double latency = application.config() ? application.config()->input.latency_offset_seconds : 0.0;
        const double judgment = application.playSession().playOptions().judgment_offset_ms / 1'000.0;
        m_judgment_offset_samples = JudgmentOffsetSamples(latency + judgment, chart.sample_rate);
        m_judge_engine = std::make_unique<core::judgement::JudgeEngine>(chart);
        m_sound_player.start(chart);
        m_lead_in.emplace(chart, m_judgment_offset_samples);
        m_current_sample = m_lead_in->sample();
        m_phase = Phase::Reveal;
        m_phase_elapsed = 0.0;
    }

    void Game::updatePlayback(const app::controller::Controller* controller, const double delta_seconds) {
        const auto& music = getData().game_loader.audio();
        if (m_phase == Phase::LeadIn) {
            m_lead_in->update(delta_seconds);
            m_current_sample = m_lead_in->sample();
            if (m_lead_in->finished()) {
                // Discard lead-in overshoot; the audio clock takes over at sample zero.
                music.play(audio::Music);
                m_phase = Phase::Starting;
                m_phase_elapsed = 0.0;
            }
        } else if (m_phase == Phase::Starting) {
            if (music.isPlaying() || music.posSample() > 0) {
                m_current_sample = Max<int64>(0, music.posSample());
                m_phase = Phase::Playing;
                m_phase_elapsed = 0.0;
            } else if (m_phase_elapsed >= AudioStartTimeout) {
                music.stop();
                m_sound_player.stop();
                m_start_error = U"Failed to start the selected audio file.";
                m_phase = Phase::Failed;
                Logger << U"[Game] " + m_start_error;
                return;
            }
        } else {
            if (!music.isPlaying()) {
                finishPlayback();
                return;
            }
            m_current_sample = music.posSample();
        }

        // A positive offset compensates late input, without moving the visual chart.
        const int64 judged_sample = JudgedSample(m_current_sample, m_judgment_offset_samples);
        readInput(controller, judged_sample);
        if (m_autoplay) {
            processJudgements(m_judge_engine->advanceAuto(m_current_sample));
        } else {
            processJudgements(m_judge_engine->advance(
                judged_sample,
                std::span{ m_input.touches.data(), m_input.touch_count },
                std::span{ m_input.side_down.data(), m_input.side_count },
                m_input.pressed.slider_pressed,
                m_input.pressed.side_pressed
            ));
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
        m_playfield_renderer.addJudgements(*getData().game_loader.chart(), events);
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
        m_phase = Phase::Finished;

        if (data.game_selection) {
            const auto& selection = *data.game_selection;
            auto& session = data.application->playSession();
            const Optional<uint32> previous_rating = session.user() ? Optional{ session.user()->rating } : none;
            if (!session.completePlay(
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
                .previous_rating = previous_rating,
                .rating = session.user() ? Optional{ session.user()->rating } : none,
            };
        }
        changeScene(SceneState::Result, 500, CrossFade::No);
    }

    void Game::draw() const {
        const auto& loader = getData().game_loader;
        if (loader.state() == game::GameLoader::State::Failed || m_phase == Phase::Failed) {
            Scene::Rect().draw(theme::Palette::White);
            drawFailure();
        } else if (m_phase == Phase::Intro) {
            drawIntro(m_elapsed, 0.0);
        } else if (loader.state() == game::GameLoader::State::Ready && loader.chart() && loader.projection()) {
            Scene::Rect().draw(theme::Palette::White);
            drawReady();
            if (m_phase == Phase::Reveal) {
                drawIntro(m_elapsed, Clamp(m_phase_elapsed / components::GameStartRevealDuration, 0.0, 1.0));
            }
        } else {
            Scene::Rect().draw(Palette::Black);
        }
    }

    void Game::drawIntro(const double elapsed, const double reveal, const bool draw_jacket) const {
        if (const auto& selection = getData().game_selection) {
            components::DrawGameStart(
                {
                    .title = selection->title,
                    .artist = selection->artist,
                    .designer = selection->difficulty.designer,
                    .jacket = m_start_jacket,
                    .difficulty_index = selection->difficulty.index,
                },
                elapsed,
                reveal,
                draw_jacket
            );
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
            const auto& result = getData().game_result;
            const uint32 display_rating = result ? result->previous_rating.value_or(user->rating) : user->rating;
            components::DrawUserNameplate(
                {
                    .display_name = user->display_name,
                    .rating = display_rating,
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
        if (m_last_judgement && m_judgement_display_seconds < JudgementDisplayDuration) {
            const Vec2 feedback_position{ DesignSize.x / 2.0, 780 };
            const double pop = std::sin(Math::Pi * Clamp(m_judgement_display_seconds / JudgementPopDuration, 0.0, 1.0));
            const double scale = 1.0 + JudgementPopScale * pop * pop;
            const double opacity =
                Clamp((JudgementDisplayDuration - m_judgement_display_seconds) / JudgementFadeDuration, 0.0, 1.0);
            const Transformer2D transform{ Mat3x2::Scale(scale, feedback_position) };
            TextureAsset{ JudgmentTexture(m_last_judgement->grade) }.drawAt(feedback_position, ColorF{ 1.0, opacity });
            const auto grade = m_last_judgement->grade;
            if ((grade == core::judgement::Grade::Great || grade == core::judgement::Grade::Good) &&
                m_last_judgement->offset_samples != 0) {
                const bool late = m_last_judgement->offset_samples > 0;
                FontAsset{ assets::font::Display }(late ? U"LATE" : U"FAST")
                    .drawAt(
                        22,
                        feedback_position.movedBy(0, 48),
                        (late ? ColorF{ U"#FF708F" } : theme::Palette::Cyan).withA(opacity)
                    );
            }
        }
    }

    void Game::drawFailure() const {
        const Vec2 center{ DesignSize.x / 2.0, DesignSize.y / 2.0 };
        const auto& error = m_start_error.isEmpty() ? getData().game_loader.error() : m_start_error;
        FontAsset{ assets::font::Text }(error).drawAt(24, center, theme::Palette::Gray);
    }

    void Game::drawFadeIn(const double t) const {
        if (const auto& transition = getData().music_start;
            transition && getData().game_loader.state() != game::GameLoader::State::Failed) {
            const double progress = Clamp(t, 0.0, 1.0);
            drawIntro(progress * components::GameStartEnterDuration, 0.0, false);
            transition->draw(0.5 + progress * 0.5);
            return;
        }
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
