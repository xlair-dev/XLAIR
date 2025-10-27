#include "InitializeApp.hpp"
#include "app/consts/Scene.hpp"

#include "app/di/Addon.hpp"
#include "app/di/SceneController.hpp"

namespace app {
    void InitializeApp(App& app, const app::types::Config& config) {
        System::SetTerminationTriggers(UserAction::CloseButtonClicked);

#if SIV3D_PLATFORM(WINDOWS)
        Platform::Windows::TextInput::DisableIME();
#endif

        RegisterAddons();
        InitializeSceneController(app.sceneController, app::types::SceneState::Title);

        using core::features::SheetRepository;
        Addon::Register(SheetRepository::Name, std::make_unique<SheetRepository>(config.sheet_provider));
        app.sceneController.get().get()->sheetRepository = Addon::GetAddon<core::features::SheetRepository>(core::features::SheetRepository::Name);

        Window::SetTitle(U"XLAIR");
        Window::Resize(config.window.width, config.window.height);
        Window::SetStyle(config.window.sizable ? WindowStyle::Sizable : WindowStyle::Fixed);
        Window::SetFullscreen(config.window.fullscreen);

        Scene::Resize(consts::SceneSize);
        Scene::SetResizeMode(ResizeMode::Keep);
        Scene::SetBackground(Palette::White);
    }
}
