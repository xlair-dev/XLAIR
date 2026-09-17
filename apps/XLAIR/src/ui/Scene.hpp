#pragma once

#include "app/Application.hpp"
#include "app/BootFlow.hpp"
#include "ui/assets/JacketRepository.hpp"

#include <utility>

namespace xlair::ui {
    enum class SceneState {
        Boot,
        Title,
    };

    struct SceneData {
        SceneData(std::shared_ptr<app::Application> application, std::shared_ptr<app::BootFlow> boot_flow)
            : application{ std::move(application) }, boot_flow{ std::move(boot_flow) } {}

        std::shared_ptr<app::Application> application;
        std::shared_ptr<app::BootFlow> boot_flow;
        assets::JacketRepository jackets;
    };

    using SceneManager = s3d::SceneManager<SceneState, SceneData>;
    using SceneBase = SceneManager::Scene;

    SceneManager CreateSceneManager(
        const std::shared_ptr<app::Application>& application,
        const std::shared_ptr<app::BootFlow>& boot_flow
    );
}
