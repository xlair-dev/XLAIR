#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "app/usecases/InitializeApp.hpp"
#include "app/usecases/LoadConfig.hpp"
#include "app/interfaces/IConfigProvider.hpp"
#include "infra/config/TomlConfigProvider.hpp"
#include "app/di/SceneController.hpp"

void Main() {
    app::App app;

    Array<std::shared_ptr<app::interfaces::IConfigProvider>> providers;
    providers.emplace_back(std::make_shared<infra::config::TomlConfigProvider>(U"config.toml"));

    const auto config = app::LoadConfig(providers);

    app::InitializeApp(app, config);
    app::InitializeSceneController(app.sceneController);

    while (System::Update() and app.sceneController.update()) {
    }
}
