#pragma once

#include "app/Application.hpp"
#include "app/card/Types.hpp"
#include "app/flows/Boot.hpp"
#include "ui/assets/JacketAssets.hpp"

#include <utility>

namespace xlair::ui {
    enum class SceneState {
        Boot,
        Title,
        Login,
    };

    struct SceneData {
        SceneData(std::shared_ptr<app::Application> application, std::shared_ptr<app::flows::Boot> boot_flow)
            : application{ std::move(application) }, boot_flow{ std::move(boot_flow) } {}

        std::shared_ptr<app::Application> application;
        std::shared_ptr<app::flows::Boot> boot_flow;
        Optional<app::card::Card> scanned_card;
        assets::JacketAssets jackets;
    };

    using SceneManager = s3d::SceneManager<SceneState, SceneData>;
    using SceneBase = SceneManager::Scene;

    SceneManager CreateSceneManager(
        const std::shared_ptr<app::Application>& application,
        const std::shared_ptr<app::flows::Boot>& boot_flow
    );
}
