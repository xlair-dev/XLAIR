#include "SceneController.hpp"
#include "app/usecases/App.hpp"
#include "app/types/Scene.hpp"

// di
#include "ui/scenes/Title.hpp"
#include "ui/scenes/Login.hpp"
#include "ui/scenes/MusicSelect.hpp"

namespace app {
    using SceneState = app::types::SceneState;

    void InitializeSceneController(SceneController& controller, SceneState initial_scene) {
        controller
            .add<ui::Title>(SceneState::Title)
            .add<ui::Login>(SceneState::Login)
            .add<ui::MusicSelect>(SceneState::MusicSelect);

        controller.init(initial_scene);
    }
}
