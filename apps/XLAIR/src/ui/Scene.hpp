#pragma once

#include "app/Application.hpp"
#include "app/card/Types.hpp"
#include "app/flows/Boot.hpp"
#include "app/flows/Maintenance.hpp"
#include "ui/MusicSelectContext.hpp"
#include "ui/assets/JacketAssets.hpp"
#include "ui/game/GameLoader.hpp"
#include "ui/game/GameResult.hpp"

#include <memory>
#include <utility>

namespace xlair::ui {
    enum class SceneState {
        Boot,
        Title,
        Login,
        MusicSelect,
        Settings,
        Game,
        Result,
        FinalResult,
        Maintenance,
        MaintenanceController,
        MaintenanceInputTest,
        MaintenanceLightTest,
        MaintenanceLatencyCalibration,
        MaintenanceCardReaderTest,
        MaintenanceServer,
        MaintenanceSound,
#ifndef NDEBUG
        ComponentGallery,
        PlayfieldDebug,
#endif
    };

    struct SceneData {
        SceneData(
            std::shared_ptr<app::Application> application,
            std::shared_ptr<app::flows::Boot> boot_flow,
            std::shared_ptr<app::flows::Maintenance> maintenance_flow
        )
            : application{ std::move(application) }, boot_flow{ std::move(boot_flow) },
              maintenance_flow{ std::move(maintenance_flow) } {}

        MusicSelectContext& ensureMusicSelectContext();

        std::shared_ptr<app::Application> application;
        std::shared_ptr<app::flows::Boot> boot_flow;
        std::shared_ptr<app::flows::Maintenance> maintenance_flow;
        Optional<app::card::Card> scanned_card;
        assets::JacketAssets jackets;
        std::unique_ptr<MusicSelectContext> music_select_context;
        Optional<GameSelection> game_selection;
        Optional<GameResult> game_result;
        game::GameLoader game_loader;
        bool returning_from_game = false;
        std::size_t maintenance_menu_index = 0;
        std::size_t maintenance_controller_index = 0;
        SceneState current_scene = SceneState::Boot;
    };

    using SceneManager = s3d::SceneManager<SceneState, SceneData>;
    class SceneBase : public SceneManager::Scene {
    public:
        explicit SceneBase(const InitData& init);
    };

    [[nodiscard]]
    bool IsMaintenanceScene(SceneState state) noexcept;

    bool HandleMaintenanceShortcut(SceneManager& scene_manager);

    SceneManager CreateSceneManager(
        const std::shared_ptr<app::Application>& application,
        const std::shared_ptr<app::flows::Boot>& boot_flow,
        const std::shared_ptr<app::flows::Maintenance>& maintenance_flow
    );
}
