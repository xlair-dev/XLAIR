#include "Scene.hpp"

#include "scenes/Boot.hpp"
#include "scenes/Title.hpp"
#include "scenes/Login.hpp"
// #include "scenes/Tutorial.hpp"
#include "scenes/MusicSelect.hpp"
#include "scenes/Settings.hpp"
#include "scenes/Game.hpp"
#include "scenes/Maintenance.hpp"

#include "scenes/ComponentGallery.hpp"
#ifndef NDEBUG
#include "scenes/PlayfieldDebug.hpp"
#endif

namespace xlair::ui {
    SceneBase::SceneBase(const InitData& init) : SceneManager::Scene{ init } {
        getData().current_scene = init.state;
    }

    bool HandleMaintenanceShortcut(SceneManager& scene_manager) {
        const auto data = scene_manager.get();
        if (data->current_scene == SceneState::Boot || data->current_scene == SceneState::Maintenance) {
            return true;
        }

        const auto* controller = data->application->controller();
        if (controller && controller->maintenanceButton(app::controller::MaintenanceButton::Button1).down()) {
            return scene_manager.changeScene(SceneState::Maintenance, 0);
        }
        return true;
    }

    MusicSelectContext& SceneData::ensureMusicSelectContext() {
        if (!music_select_context) {
            const auto& config = application->config();
            const double remaining_seconds = config ? config->system.menu_timer_seconds : 0.0;
            music_select_context = std::make_unique<MusicSelectContext>(application->musicCatalog(), remaining_seconds);
        }
        return *music_select_context;
    }

    SceneManager CreateSceneManager(
        const std::shared_ptr<app::Application>& application,
        const std::shared_ptr<app::flows::Boot>& boot_flow
    ) {
        scenes::MusicSelect::RegisterAssets();
        scenes::Settings::RegisterAssets();

        SceneManager scene_manager{ std::make_shared<SceneData>(application, boot_flow) };

        scene_manager
            .add<scenes::Boot>(SceneState::Boot)               // initialize game
            .add<scenes::Title>(SceneState::Title)             // title; wait user, scan card
            .add<scenes::Login>(SceneState::Login)             // login; cancel -> Title, ok -> MusicSelect
            .add<scenes::MusicSelect>(SceneState::MusicSelect) // music
            .add<scenes::Settings>(SceneState::Settings)       // settings
            .add<scenes::Game>(SceneState::Game)               // main game screen
            .add<scenes::Maintenance>(SceneState::Maintenance);
#ifndef NDEBUG
        scene_manager
            .add<scenes::ComponentGallery>(SceneState::ComponentGallery) // check ui
            .add<scenes::PlayfieldDebug>(SceneState::PlayfieldDebug);
#endif
        scene_manager.init(SceneState::Boot, 0);

        return scene_manager;
    }
}
