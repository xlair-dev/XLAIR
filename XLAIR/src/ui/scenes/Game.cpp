#include "Game.hpp"

#include "ui/components/PlayerNameplate.hpp"
#include "ui/components/GameMusicPlate.hpp"

namespace ui {
    Game::Game(const InitData& init) : IScene(init) {}
    Game::~Game() {}

    void Game::update() {}
    void Game::draw() const {
        auto& data = getData();

        components::DrawPlayerNameplate(getData().playerData, Point{ 59, 72 });
        components::DrawGameMusicPlate(Point{ 1500, 72 }, data.sheetRepository.getMetadata(data.playerData.selected_index), data.sheetRepository.getJacket(data.playerData.selected_index).value(), data.playerData.selected_difficulty, 0.0, 1);
    }
}
