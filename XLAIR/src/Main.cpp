#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "app/usecases/LoadConfig.hpp"
#include "app/usecases/InitializeApp.hpp"
#include "app/usecases/Assets.hpp"

#include "app/di/Providers.hpp"

#include "core/features/ControllerManager.hpp"

void Main() {
    app::App app;

    const auto config = app::LoadConfig(app::MakeDefaultConfigProviders());
    app::InitializeApp(app, config);

    auto* scene_data = app.sceneController.get().get();

    app::assets::Initialize();

    if (not core::features::ControllerManager::InitializeDevice()) {
        throw Error{ U"InitializeDevice failed." };
    }

    while (System::Update() and app.sceneController.update()) {
    }
}
