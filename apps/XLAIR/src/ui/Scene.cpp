#include "Scene.hpp"

#include "assets/Assets.hpp"
#include "theme/Palette.hpp"

#include "scenes/Boot.hpp"
#include "scenes/Title.hpp"
#include "scenes/Login.hpp"
// #include "scenes/Tutorial.hpp"
#include "scenes/MusicSelect.hpp"
#include "scenes/Settings.hpp"
#include "scenes/Game.hpp"
#include "scenes/Result.hpp"
#include "scenes/FinalResult.hpp"
#include "scenes/Maintenance.hpp"
#include "scenes/maintenance/ControllerMenu.hpp"
#include "scenes/maintenance/InputTest.hpp"
#include "scenes/maintenance/LightTest.hpp"
#include "scenes/maintenance/LatencyCalibration.hpp"
#include "scenes/maintenance/CardReaderTest.hpp"
#include "scenes/maintenance/Server.hpp"
#include "scenes/maintenance/Sound.hpp"

#ifndef NDEBUG
#include "scenes/debug/ComponentGallery.hpp"
#include "scenes/debug/PlayfieldDebug.hpp"
#endif

namespace xlair::ui {
    SceneBase::SceneBase(const InitData& init) : SceneManager::Scene{ init } {
        getData().current_scene = init.state;
    }

    bool IsMaintenanceScene(const SceneState state) noexcept {
        switch (state) {
            case SceneState::Maintenance:
            case SceneState::MaintenanceController:
            case SceneState::MaintenanceInputTest:
            case SceneState::MaintenanceLightTest:
            case SceneState::MaintenanceLatencyCalibration:
            case SceneState::MaintenanceCardReaderTest:
            case SceneState::MaintenanceServer:
            case SceneState::MaintenanceSound:
                return true;
            default:
                return false;
        }
    }

    void DrawSceneOverlay(const SceneData& data) {
        if (data.current_scene == SceneState::Game || data.title_to_login_wipe) {
            return;
        }
#ifndef NDEBUG
        if (data.current_scene == SceneState::PlayfieldDebug) {
            return;
        }
#endif
        FontAsset{ assets::font::Label }(U"CREDIT(S): {}"_fmt(data.application->creditPool().pending()))
            .draw(18, Arg::bottomRight(Scene::Width() - 10, Scene::Height() - 32), theme::Palette::Gray);
    }

    bool HandleMaintenanceShortcut(SceneManager& scene_manager) {
        const auto data = scene_manager.get();
        // Do not discard a completed session or interrupt its pending save via Maintenance.
        if (data->title_to_login_wipe || data->current_scene == SceneState::Boot ||
            data->current_scene == SceneState::Login || IsMaintenanceScene(data->current_scene) ||
            data->current_scene == SceneState::FinalResult ||
            (data->current_scene == SceneState::Result && data->application->playSession().finished())) {
            return true;
        }

        const auto* controller = data->application->controller();
        if (controller && controller->maintenanceButton(app::controller::MaintenanceButton::Button1).down()) {
            data->maintenance_menu_index = 0;
            data->maintenance_controller_index = 0;
            data->maintenance_flow->reset();
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
        const std::shared_ptr<app::flows::Boot>& boot_flow,
        const std::shared_ptr<app::flows::Maintenance>& maintenance_flow
    ) {
        scenes::Login::RegisterAssets();
        scenes::MusicSelect::RegisterAssets();
        scenes::Settings::RegisterAssets();

        SceneManager scene_manager{ std::make_shared<SceneData>(application, boot_flow, maintenance_flow) };

        scene_manager
            .add<scenes::Boot>(SceneState::Boot)               // initialize game
            .add<scenes::Title>(SceneState::Title)             // title; wait user, scan card
            .add<scenes::Login>(SceneState::Login)             // login; cancel -> Title, ok -> MusicSelect
            .add<scenes::MusicSelect>(SceneState::MusicSelect) // music
            .add<scenes::Settings>(SceneState::Settings)       // settings
            .add<scenes::Game>(SceneState::Game)               // main game screen
            .add<scenes::Result>(SceneState::Result)           // result of the completed play
            .add<scenes::FinalResult>(SceneState::FinalResult) // results of the whole session
            .add<scenes::Maintenance>(SceneState::Maintenance)
            .add<scenes::maintenance::ControllerMenu>(SceneState::MaintenanceController)
            .add<scenes::maintenance::InputTest>(SceneState::MaintenanceInputTest)
            .add<scenes::maintenance::LightTest>(SceneState::MaintenanceLightTest)
            .add<scenes::maintenance::LatencyCalibration>(SceneState::MaintenanceLatencyCalibration)
            .add<scenes::maintenance::CardReaderTest>(SceneState::MaintenanceCardReaderTest)
            .add<scenes::maintenance::Server>(SceneState::MaintenanceServer)
            .add<scenes::maintenance::Sound>(SceneState::MaintenanceSound);
#ifndef NDEBUG
        scene_manager
            .add<scenes::debug::ComponentGallery>(SceneState::ComponentGallery) // check ui
            .add<scenes::debug::PlayfieldDebug>(SceneState::PlayfieldDebug);
#endif
        scene_manager.init(SceneState::Boot, 0);

        return scene_manager;
    }
}
