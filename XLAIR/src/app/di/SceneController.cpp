#include "SceneController.hpp"
#include "core/types/Scene.hpp"
#include "app/usecases/App.hpp"

// di
#include "ui/scenes/Title.hpp"
#include "ui/scenes/Login.hpp"

namespace app {
    void InitializeSceneController(SceneController& controller) {
        controller
            .add<ui::Title>(core::SceneState::Title)
            .add<ui::Login>(core::SceneState::Login);

        controller.init(core::SceneState::Title);
    }
}
