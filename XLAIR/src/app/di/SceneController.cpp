#include "SceneController.hpp"
#include "app/usecases/App.hpp"
#include "app/types/Scene.hpp"

// di
#include "ui/scenes/Title.hpp"
#include "ui/scenes/Login.hpp"

namespace app {
    void InitializeSceneController(SceneController& controller) {
        using SceneState = app::types::SceneState;
        controller
            .add<ui::Title>(SceneState::Title)
            .add<ui::Login>(SceneState::Login);

        controller.init(SceneState::Title);
    }
}
