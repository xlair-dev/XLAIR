#include "Scene.hpp"

#include "scenes/Boot.hpp"
#include "scenes/Title.hpp"

namespace xlair::ui {
    SceneManager CreateSceneManager(const std::shared_ptr<app::Application>& application) {
        SceneManager scene_manager{ application };

        scene_manager
            .add<scenes::Boot>(SceneState::Boot)    // initialize game
            .add<scenes::Title>(SceneState::Title); // title; wait user
        scene_manager.init(SceneState::Boot, 0);

        return scene_manager;
    }
}
