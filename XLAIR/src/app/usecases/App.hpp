#pragma once
#include "Common.hpp"
#include "app/types/Scene.hpp"
#include "app/interfaces/IApiClient.hpp"
#include "core/features/SheetRepository.hpp"
#include "core/types/UserData.hpp"
#include "core/types/Scoring.hpp"

namespace app {

    struct GameData {
        core::features::SheetRepository* sheetRepository;
        core::types::UserData userData;
        core::types::Score score;
        std::shared_ptr<interfaces::IApiClient> api;
    };

    using SceneController = SceneManager<app::types::SceneState, GameData>;

    struct App {
        using Scene = SceneController::Scene;
        SceneController sceneController;
    };

}
