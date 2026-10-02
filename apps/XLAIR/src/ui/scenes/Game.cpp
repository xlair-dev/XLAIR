#include "Game.hpp"

#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    Game::Game(const InitData& init) : SceneBase{ init } {
        if (getData().music_select_context) {
            getData().music_select_context->pause();
        }
        if (const auto& selection = getData().game_selection) {
            getData().game_loader.start(*selection);
        }
    }

    Game::~Game() {
        getData().game_loader.cancel();
        getData().game_selection.reset();
    }

    void Game::update() {
        if (KeyEscape.pressedDuration() < SecondsF{ 1.5 }) {
            return;
        }

        getData().returning_from_game = true;
        changeScene(SceneState::MusicSelect, 500, CrossFade::No);
    }

    void Game::draw() const {
        Scene::Rect().draw(theme::Palette::White);
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
                message = U"Chart ready (gameplay is not implemented yet).";
                break;
            case game::GameLoader::State::Failed:
                message = loader.error();
                break;
        }
        FontAsset{ assets::font::Text }(message).drawAt(24, center.movedBy(0, 115), theme::Palette::Gray);
        FontAsset{ assets::font::Text }(U"Hold Esc to return to MUSIC SELECT")
            .drawAt(20, center.movedBy(0, 180), theme::Palette::Gray);
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
