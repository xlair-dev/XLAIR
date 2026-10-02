#include "Scene.hpp"

#include "scenes/Boot.hpp"
#include "scenes/Title.hpp"
#include "scenes/Login.hpp"
// #include "scenes/Tutorial.hpp"
#include "scenes/MusicSelect.hpp"
#include "scenes/Settings.hpp"
#include "scenes/Game.hpp"

#include "scenes/ComponentGallery.hpp"

namespace xlair::ui {
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
            .add<scenes::Boot>(SceneState::Boot)                          // initialize game
            .add<scenes::Title>(SceneState::Title)                        // title; wait user, scan card
            .add<scenes::Login>(SceneState::Login)                        // login; cancel -> Title, ok -> MusicSelect
            .add<scenes::MusicSelect>(SceneState::MusicSelect)            // music
            .add<scenes::Settings>(SceneState::Settings)                  // settings
            .add<scenes::Game>(SceneState::Game)                          // main game screen
            .add<scenes::ComponentGallery>(SceneState::ComponentGallery); // test
        scene_manager.init(SceneState::Boot, 0);

        return scene_manager;
    }
}
