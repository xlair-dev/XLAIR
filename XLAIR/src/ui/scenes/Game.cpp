#include "Game.hpp"

namespace ui {
    Game::Game(const InitData& init) : IScene(init) {}
    Game::~Game() {}

    void Game::update() {}
    void Game::draw() const {}
}
