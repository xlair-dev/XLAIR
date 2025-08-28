#include "SceneController.hpp"
#include "app/usecases/App.hpp"
#include "core/types/Scene.hpp"

// di
#include "ui/scenes/Title.hpp"
#include "ui/scenes/Login.hpp"

namespace app {
    void InitializeSceneController(SceneController& controller) {
        using SceneState = core::types::SceneState;
        controller
            .add<ui::Title>(SceneState::Title)
            .add<ui::Login>(SceneState::Login);

        controller.init(SceneState::Title);
    }
}
