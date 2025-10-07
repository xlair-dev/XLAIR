#include "Game.hpp"

#include "ui/components/PlayerNameplate.hpp"

namespace ui {
    Game::Game(const InitData& init) : IScene(init) {}
    Game::~Game() {}

    void Game::update() {}
    void Game::draw() const {
        components::DrawPlayerNameplate(getData().playerData, Point{ 59, 72 });
    }
}
