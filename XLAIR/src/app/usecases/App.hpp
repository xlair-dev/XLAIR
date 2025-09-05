#pragma once
#include "Common.hpp"
#include "app/types/Scene.hpp"
#include "core/features/SheetRepository.hpp"

namespace app {

    struct GameData {
        core::features::SheetRepository sheetRepository;
    };

    using SceneController = SceneManager<app::types::SceneState, GameData>;

    struct App {
        using Scene = SceneController::Scene;
        SceneController sceneController;
    };

}
