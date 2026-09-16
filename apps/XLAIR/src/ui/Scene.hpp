#pragma once

#include "app/Application.hpp"
#include "app/BootFlow.hpp"

namespace xlair::ui {
    enum class SceneState {
        Boot,
        Title,
    };

    struct SceneData {
        std::shared_ptr<app::Application> application;
        std::shared_ptr<app::BootFlow> boot_flow;
    };

    using SceneManager = s3d::SceneManager<SceneState, SceneData>;
    using SceneBase = SceneManager::Scene;

    SceneManager CreateSceneManager(
        const std::shared_ptr<app::Application>& application,
        const std::shared_ptr<app::BootFlow>& boot_flow
    );
}
