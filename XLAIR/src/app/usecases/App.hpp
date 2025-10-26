#pragma once
#include "Common.hpp"
#include "app/types/Scene.hpp"
#include "core/features/SheetRepository.hpp"
#include "core/types/PlayerData.hpp"
#include "core/types/Scoring.hpp"

namespace app {

    struct GameData {
        core::features::SheetRepository* sheetRepository;
        core::types::PlayerData playerData;
        core::types::Score score;
    };

    using SceneController = SceneManager<app::types::SceneState, GameData>;

    struct App {
        using Scene = SceneController::Scene;
        SceneController sceneController;
    };

}
