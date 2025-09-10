#include "InitializeApp.hpp"
#include "app/consts/Scene.hpp"

namespace app {
    void InitializeApp(App& app, const app::types::Config& config) {
        System::SetTerminationTriggers(UserAction::CloseButtonClicked);

#if SIV3D_PLATFORM(WINDOWS)
        Platform::Windows::TextInput::DisableIME();
#endif

        Window::SetTitle(U"XLAIR");
        Window::Resize(config.window.width, config.window.height);
        Window::SetStyle(config.window.sizable ? WindowStyle::Sizable : WindowStyle::Fixed);
        Window::SetFullscreen(config.window.fullscreen);

        Scene::Resize(consts::SceneSize);
        Scene::SetResizeMode(ResizeMode::Keep);
        // Scene::SetBackground(Palette::White);
    }
}
