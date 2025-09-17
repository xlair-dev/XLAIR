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

    // TODO: api mock
    app.sceneController.get().get()->playerData.username = U"User Name";
    app.sceneController.get().get()->playerData.displayname = U"Display Name";
    app.sceneController.get().get()->playerData.rating = 200;
    app.sceneController.get().get()->playerData.level = 7;

    // TODO: asset management
    FontAsset::Register(U"tile", FontMethod::Bitmap, 38, U"assets/fonts/Jost/Jost-Medium.ttf");
    FontAsset::Load(U"tile");
    FontAsset::Register(U"tile.text", FontMethod::MSDF, 75, U"assets/fonts/Jost/Jost-Regular.ttf");
    FontAsset::Load(U"tile.text");
    const Font cjk { FontMethod::MSDF, 75, Typeface::CJK_Regular_JP };
    FontAsset(U"tile.text").addFallback(cjk);

    FontAsset::Register(U"BrunoAce", FontMethod::Bitmap, 68, U"assets/fonts/BrunoAce/BrunoAce-Regular.ttf");
    FontAsset::Load(U"BrunoAce");

    PixelShaderAsset::Register(U"grayscale", HLSL{ U"assets/shaders/grayscale.hlsl", U"PS" } | GLSL{ U"assets/shaders/grayscale.frag", {{U"PSConstants2D", 0}} });
    PixelShaderAsset::Load(U"grayscale");

    app::InitializeSceneController(app.sceneController, app::types::SceneState::MusicSelect);
    while (System::Update() and app.sceneController.update()) {
    }
}
