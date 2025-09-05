#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "app/usecases/LoadConfig.hpp"
#include "app/usecases/InitializeApp.hpp"

#include "app/di/SceneController.hpp"
#include "app/di/Providers.hpp"

#include "infra/sheet/SheetMock.hpp"

void Main() {
    app::App app;

    const auto config = app::LoadConfig(app::MakeDefaultConfigProviders());
    app::InitializeApp(app, config);
    app::InitializeSceneController(app.sceneController);

    app.sceneController.get().get()->sheetRepository.load(std::make_unique<infra::sheet::SheetMock>(5));
    app.sceneController.get().get()->sheetRepository.loadJacket();
    while (System::Update() and app.sceneController.update()) {
    }
}
