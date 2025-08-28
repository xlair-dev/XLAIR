#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "app/usecases/LoadConfig.hpp"
#include "app/usecases/InitializeApp.hpp"

#include "app/di/SceneController.hpp"
#include "app/di/Providers.hpp"

void Main() {
    app::App app;

    const auto config = app::LoadConfig(app::MakeDefaultConfigProviders());
    app::InitializeApp(app, config);
    app::InitializeSceneController(app.sceneController);

    while (System::Update() and app.sceneController.update()) {
    }
}
