#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "app/usecases/LoadConfig.hpp"
#include "app/usecases/InitializeApp.hpp"
#include "app/usecases/Assets.hpp"

#include "app/di/Providers.hpp"

#include "infra/sheet/SheetMock.hpp"

void Main() {
    app::App app;

    const auto config = app::LoadConfig(app::MakeDefaultConfigProviders());
    app::InitializeApp(app, config);

    auto* scene_data = app.sceneController.get().get();

    // TODO: api mock
    scene_data->playerData.username = U"User Name";
    scene_data->playerData.displayname = U"Display Name";
    scene_data->playerData.rating = 200;
    scene_data->playerData.level = 7;

    app::assets::Initialize();

    while (System::Update() and app.sceneController.update()) {
    }
}
