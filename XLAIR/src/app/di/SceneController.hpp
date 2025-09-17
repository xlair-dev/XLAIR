#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

namespace app {
    void InitializeSceneController(SceneController& controller, types::SceneState initial_scene = types::SceneState::Title);
}
