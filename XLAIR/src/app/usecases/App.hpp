#pragma once
#include "Common.hpp"
#include "core/types/Scene.hpp"

namespace app {

    // TODO: 仮のゲームのデータ
    struct GameData {};

    using SceneController = SceneManager<core::types::SceneState, GameData>;

    struct App {
        using Scene = SceneController::Scene;
        SceneController sceneController;
    };

}
