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
    app.sceneController.get().get()->sheetRepository.load(std::make_unique<infra::sheet::SheetMock>(5));
    app.sceneController.get().get()->sheetRepository.loadJacket();

    // TODO: asset management
    TextureAsset::Register(U"sparkle", U"assets/textures/s2.png");
    TextureAsset::Load(U"sparkle");
    FontAsset::Register(U"tile", FontMethod::Bitmap, 76, U"assets/fonts/Jost/Jost-Medium.ttf");
    FontAsset::Load(U"tile");
    FontAsset::Register(U"tile.text", FontMethod::Bitmap, 150, U"assets/fonts/Jost/Jost-Regular.ttf");
    FontAsset::Load(U"tile.text");
    FontAsset::Register(U"tile.cjk", 150, Typeface::CJK_Regular_JP);
    const Font cjk { 150, Typeface::CJK_Regular_JP };
    FontAsset(U"tile.text").addFallback(cjk);

    FontAsset::Register(U"BrunoAce", FontMethod::Bitmap, 70, U"assets/fonts/Bruno_Ace/BrunoAce-Regular.ttf");
    FontAsset::Load(U"BrunoAce");

    app::InitializeSceneController(app.sceneController, app::types::SceneState::MusicSelect);
    while (System::Update() and app.sceneController.update()) {
    }
}
