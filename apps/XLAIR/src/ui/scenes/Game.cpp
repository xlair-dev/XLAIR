#include "Game.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/theme/Palette.hpp"

#include <cmath>

namespace xlair::ui::scenes {
    namespace {
        constexpr auto CancelButton = app::controller::MaintenanceButton::Button2;
        constexpr Rect PlayfieldViewport{ 360, 90, 1200, 880 };
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
            m_pixels_per_second *= Clamp(speed, 0.25, 10.0);
        }
    }

    Game::~Game() {
        getData().game_loader.cancel();
        getData().game_selection.reset();
    }

    void Game::update() {
        m_elapsed += Scene::DeltaTime();
        const auto* controller = getData().application->controller();
        const bool cancel = controller && controller->maintenanceButton(CancelButton).down();
        if (cancel || KeyEscape.pressedDuration() >= SecondsF{ 1.5 }) {
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
            music.play(audio::Music);
            m_playback_started = true;
            return;
        }

        if (!m_playback_finished) {
            if (music.isPlaying()) {
                m_current_sample = music.posSample();
            } else {
                m_playback_finished = true;
            }
        }
    }

    void Game::draw() const {
        Scene::Rect().draw(theme::Palette::White);
        const auto& loader = getData().game_loader;
        if (loader.state() == game::GameLoader::State::Ready && loader.chart() && loader.projection()) {
            drawReady();
        } else {
            drawLoading();
        }

        FontAsset{ assets::font::Text }(U"Maintenance 2 (F3) or hold Esc: MUSIC SELECT")
            .drawAt(20, Vec2{ DesignSize.x / 2.0, 1050 }, theme::Palette::Gray);
    }

    void Game::drawReady() const {
        const auto& loader = getData().game_loader;
        m_playfield_renderer
            .draw(*loader.chart(), *loader.projection(), m_current_sample, PlayfieldViewport, m_pixels_per_second);
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
        // Judgement and scoring are not connected yet; keep the legacy HUD layout ready for them.
        m_score_bar.draw({}, Point{ DesignSize.x / 2 - 434, 56 });
        const StringView status = m_playback_finished ? U"Playback complete" : U"Playing";
        FontAsset{ assets::font::Text }(status).drawAt(24, Vec2{ DesignSize.x / 2.0, 995 }, theme::Palette::Gray);
    }

    void Game::drawLoading() const {
        const Vec2 center{ DesignSize.x / 2.0, DesignSize.y / 2.0 };
        FontAsset{ assets::font::Display }(U"GAME").drawAt(72, center.movedBy(0, -90), theme::Palette::Gray);
        if (const auto& selection = getData().game_selection) {
            FontAsset{ assets::font::Text }(selection->title).drawAt(32, center, theme::Palette::Gray);
            FontAsset{ assets::font::Text }(selection->difficulty.id)
                .drawAt(24, center.movedBy(0, 45), theme::Palette::Gray);
        }

        const auto& loader = getData().game_loader;
        String message;
        switch (loader.state()) {
            case game::GameLoader::State::Idle:
                message = U"No chart selected.";
                break;
            case game::GameLoader::State::LoadingAudio:
                message = U"Loading audio...";
                break;
            case game::GameLoader::State::LoadingChart:
                message = U"Loading chart...";
                break;
            case game::GameLoader::State::Ready:
                message = U"Chart ready.";
                break;
            case game::GameLoader::State::Failed:
                message = loader.error();
                break;
        }
        FontAsset{ assets::font::Text }(message).drawAt(24, center.movedBy(0, 115), theme::Palette::Gray);
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
