#pragma once

#include "app/Application.hpp"

namespace xlair::ui {
    enum class SceneState {
        Boot,
        Title,
    };

    using SceneManager = s3d::SceneManager<SceneState, app::Application>;
    using SceneBase = SceneManager::Scene;

    SceneManager CreateSceneManager(const std::shared_ptr<app::Application>& application);
}
