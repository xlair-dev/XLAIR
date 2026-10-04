#pragma once

#include "app/Application.hpp"
#include "app/card/Types.hpp"
#include "app/flows/Boot.hpp"
#include "ui/MusicSelectContext.hpp"
#include "ui/assets/JacketAssets.hpp"
#include "ui/game/GameLoader.hpp"

#include <memory>
#include <utility>

namespace xlair::ui {
    enum class SceneState {
        Boot,
        Title,
        Login,
        MusicSelect,
        Settings,
        Game,
        ComponentGallery,
#ifndef NDEBUG
        PlayfieldDebug,
#endif
    };

    struct SceneData {
        SceneData(std::shared_ptr<app::Application> application, std::shared_ptr<app::flows::Boot> boot_flow)
            : application{ std::move(application) }, boot_flow{ std::move(boot_flow) } {}

        MusicSelectContext& ensureMusicSelectContext();

        std::shared_ptr<app::Application> application;
        std::shared_ptr<app::flows::Boot> boot_flow;
        Optional<app::card::Card> scanned_card;
        assets::JacketAssets jackets;
        std::unique_ptr<MusicSelectContext> music_select_context;
        Optional<GameSelection> game_selection;
        game::GameLoader game_loader;
        bool returning_from_game = false;
    };

    using SceneManager = s3d::SceneManager<SceneState, SceneData>;
    using SceneBase = SceneManager::Scene;

    SceneManager CreateSceneManager(
        const std::shared_ptr<app::Application>& application,
        const std::shared_ptr<app::flows::Boot>& boot_flow
    );
}
