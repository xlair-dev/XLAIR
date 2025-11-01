#pragma once
#include "Common.hpp"
#include "app/types/Scene.hpp"
#include "app/interfaces/IApiClient.hpp"
#include "core/features/SheetRepository.hpp"
#include "core/types/UserData.hpp"
#include "core/types/Scoring.hpp"
#include "core/types/Record.hpp"

namespace app {

    struct GameData {
        int32 max_playable = 3;
        int32 playable = 3;
        int32 menu_timer = 90;
        core::features::SheetRepository* sheetRepository;
        core::types::UserData userData;
        core::types::Score score;
        std::shared_ptr<interfaces::IApiClient> api;
        HashTable<String, core::types::Record> records; // sheet_id -> record
        Array<core::types::Record> played_records;
    };

    using SceneController = SceneManager<app::types::SceneState, GameData>;

    struct App {
        using Scene = SceneController::Scene;
        SceneController sceneController;
    };

}
