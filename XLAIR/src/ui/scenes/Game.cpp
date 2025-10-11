#include "Game.hpp"

#include "ui/components/PlayerNameplate.hpp"
#include "ui/components/GameMusicPlate.hpp"

namespace ui {
    Game::Game(const InitData& init) : IScene(init) {}
    Game::~Game() {}

    void Game::update() {
        m_tile_offset_raw += Scene::DeltaTime();
        m_tile_offset = Math::Max(0.0, m_tile_offset_raw);
    }

    void Game::draw() const {
        auto& data = getData();

        components::DrawPlayerNameplate(getData().playerData, Point{ 59, 72 });
        components::DrawGameMusicPlate(Point{ 1500, 72 }, data.sheetRepository.getMetadata(data.playerData.selected_index), data.sheetRepository.getJacket(data.playerData.selected_index).value(), data.playerData.selected_difficulty, m_tile_offset, 1);
    }
}
