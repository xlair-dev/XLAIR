#include "Scene.hpp"

#include "scenes/Boot.hpp"
#include "scenes/Title.hpp"
#include "scenes/Login.hpp"
// #include "scenes/Tutorial.hpp"
#include "scenes/MusicSelect.hpp"

#include "scenes/ComponentGallery.hpp"

namespace xlair::ui {
    SceneManager CreateSceneManager(
        const std::shared_ptr<app::Application>& application,
        const std::shared_ptr<app::flows::Boot>& boot_flow
    ) {
        SceneManager scene_manager{ std::make_shared<SceneData>(application, boot_flow) };

        scene_manager
            .add<scenes::Boot>(SceneState::Boot)                          // initialize game
            .add<scenes::Title>(SceneState::Title)                        // title; wait user, scan card
            .add<scenes::Login>(SceneState::Login)                        // login; cancel -> Title, ok -> MusicSelect
            .add<scenes::MusicSelect>(SceneState::MusicSelect)            // music
            .add<scenes::ComponentGallery>(SceneState::ComponentGallery); // test
        // scene_manager.init(SceneState::Boot, 0);
        scene_manager.init(SceneState::ComponentGallery, 0);

        return scene_manager;
    }
}
