#pragma once
#include "Common.hpp"
#include "app/types/Scene.hpp"

namespace app {

    // TODO: 仮のゲームのデータ
    struct GameData {};

    using SceneController = SceneManager<app::types::SceneState, GameData>;

    struct App {
        using Scene = SceneController::Scene;
        SceneController sceneController;
    };

}
