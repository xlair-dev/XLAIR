#include "Game.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/theme/Palette.hpp"

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
        if (getData().music_select_context) {
            getData().music_select_context->pause();
        }
        if (const auto& selection = getData().game_selection) {
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
        getData().game_selection.reset();
    }

    void Game::update() {
        m_elapsed += Scene::DeltaTime();
        m_judgement_display_seconds += Scene::DeltaTime();
        const auto* controller = getData().application->controller();
        const bool cancel = controller && controller->maintenanceButton(CancelButton).down();
        if (cancel || KeyEscape.pressedDuration() >= SecondsF{ 1.5 }) {
            m_sound_player.stop();
            if (const auto& music = getData().game_loader.audio()) {
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

        const auto& music = loader.audio();
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
            if (music.isPlaying()) {
                m_current_sample = music.posSample();
                // A positive offset compensates late input; it moves touch time earlier, not the visual chart.
                const int64 judged_sample = m_current_sample - m_judgment_offset_samples;
                if (getData().application->autoplay()) {
                    const auto events = m_judge_engine->advanceAuto(m_current_sample);
                    m_sound_player.update(m_current_sample, *m_judge_engine, events);
                    if (!events.isEmpty()) {
                        m_last_judgement = events.back();
                        m_judgement_display_seconds = 0.0;
                    }
                    return;
                }
                std::array<core::judgement::TouchEvent, app::controller::TouchZoneCount * 2> touches;
                std::array<core::judgement::SideEvent, app::controller::SideButtonCount> side_down;
                std::size_t touch_count = 0;
                std::size_t side_count = 0;
                uint32 slider_pressed = 0;
                uint8 side_pressed = 0;
                if (controller) {
                    for (std::size_t zone = 0; zone < app::controller::TouchZoneCount; ++zone) {
                        const auto& state = controller->touchZone(zone);
                        if (state.pressed()) {
                            slider_pressed |= uint32{ 1 } << zone;
                        }
                        if (state.up()) {
                            touches[touch_count++] = {
                                .zone = static_cast<uint8>(zone),
                                .sample = judged_sample,
                                .phase = core::judgement::TouchEvent::Phase::Up,
                            };
                        }
                    }
                    // Releases precede presses within a sampled frame so a swipe that
                    // crosses sensors in one update can complete a FLICK.
                    for (std::size_t zone = 0; zone < app::controller::TouchZoneCount; ++zone) {
                        if (controller->touchZone(zone).down()) {
                            touches[touch_count++] = { .zone = static_cast<uint8>(zone), .sample = judged_sample };
                        }
                    }
                    for (std::size_t button = 0; button < app::controller::SideButtonCount; ++button) {
                        const auto& state = controller->sideButton(static_cast<app::controller::SideButton>(button));
                        if (state.pressed()) {
                            side_pressed |= uint8{ 1 } << static_cast<uint8>(SideButtonMapping[button]);
                        }
                        if (state.down()) {
                            side_down[side_count++] = { .button = SideButtonMapping[button], .sample = judged_sample };
                        }
                    }
                }
                const auto events = m_judge_engine->advance(
                    judged_sample,
                    std::span{ touches.data(), touch_count },
                    std::span{ side_down.data(), side_count },
                    slider_pressed,
                    side_pressed
                );
                m_sound_player.update(m_current_sample, *m_judge_engine, events, m_judge_engine->flickContacts());
                if (!events.isEmpty()) {
                    m_last_judgement = events.back();
                    m_judgement_display_seconds = 0.0;
                }
            } else {
                m_current_sample = static_cast<int64>(music.samples());
                // Resolve pending notes at the end even if the audio ends within a judgement window.
                Array<core::judgement::JudgementEvent> events;
                if (getData().application->autoplay()) {
                    // Cover the final frame before resolving any notes beyond the audio.
                    events = m_judge_engine->advanceAuto(m_current_sample);
                }
                events.append(
                    m_judge_engine->advance(m_current_sample + loader.chart()->sample_rate - m_judgment_offset_samples)
                );
                m_sound_player.update(m_current_sample, *m_judge_engine, events);
                if (!events.isEmpty()) {
                    m_last_judgement = events.back();
                    m_judgement_display_seconds = 0.0;
                }
                m_sound_player.finish();
                m_playback_finished = true;
            }
        }
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

    void Game::drawReady() const {
        const auto& loader = getData().game_loader;
        const auto score = m_judge_engine ? m_judge_engine->score() : core::judgement::ScoreState{};
        uint32 slider_pressed = 0;
        uint8 side_pressed = 0;
        if (const auto* controller = getData().application->controller()) {
            for (std::size_t zone = 0; zone < app::controller::TouchZoneCount; ++zone) {
                if (controller->touchZone(zone).pressed()) {
                    slider_pressed |= uint32{ 1 } << zone;
                }
            }
            for (std::size_t button = 0; button < app::controller::SideButtonCount; ++button) {
                if (controller->sideButton(static_cast<app::controller::SideButton>(button)).pressed()) {
                    side_pressed |= uint8{ 1 } << static_cast<uint8>(SideButtonMapping[button]);
                }
            }
        }
        m_playfield_renderer.draw(
            m_camera,
            *loader.chart(),
            *loader.projection(),
            m_current_sample,
            m_note_speed,
            score.combo,
            false,
            m_judge_engine.get(),
            slider_pressed,
            side_pressed
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
                    .max_plays = session.maxPlays(),
                    .remaining_plays = session.remainingPlays(),
                },
                Point{ 1480, 72 },
                m_elapsed
            );
        }
        m_score_bar.draw(
            { .score = score.score, .clear_gauge = score.clear_gauge },
            Point{ DesignSize.x / 2 - 434, 56 }
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
