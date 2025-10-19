#include "Game.hpp"

#include "app/consts/Scene.hpp"
#include "ui/components/PlayerNameplate.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/GameScoreBar.hpp"

namespace ui {
    Game::Game(const InitData& init) : IScene(init) {}
    Game::~Game() {}

    void Game::update() {
        m_tile_offset_raw += Scene::DeltaTime();
        m_tile_offset = Math::Max(0.0, m_tile_offset_raw);
    }

    void Game::draw() const {
        auto& data = getData();

        drawField();

        components::DrawPlayerNameplate(getData().playerData, Point{ 59, 72 });
        components::DrawGameMusicPlate(Point{ 1500, 72 }, data.sheetRepository.getMetadata(data.playerData.selected_index), data.sheetRepository.getJacket(data.playerData.selected_index).value(), data.playerData.selected_difficulty, m_tile_offset, 1);
        components::DrawGameScoreBar(Point{ app::consts::SceneWidth / 2 - 434, 56 }, data.score);
    }

    void Game::drawField() const {
        constexpr Quad stage{
            Vec2{ 225, app::consts::SceneHeight },
            Vec2{ 890, 0 },
            Vec2{ app::consts::SceneWidth - 890, 0},
            Vec2{ app::consts::SceneWidth - 225, app::consts::SceneHeight},
        };
        stage.draw(Palette::Black);
    }
}
