#include <Siv3D.hpp> // Siv3D v0.6.16

#include "SideMenu.hpp"

#include "addon/NotificationAddon.hpp"
#include "addon/LoadingAnimationAddon.hpp"
#include "addon/SheetManagerAddon.hpp"

void Main() {
    Addon::Register<NotificationAddon>(NotificationAddon::Name);
    Addon::Register<LoadingAnimationAddon>(LoadingAnimationAddon::Name);
    Addon::Register<SheetManagerAddon>(SheetManagerAddon::Name);

    // Consider: https://qiita.com/m4saka/items/5da6cd4b57bc894d35dd
    // Graphics::SetVSyncEnabled(false);
    Window::SetStyle(WindowStyle::Sizable);
    Window::Resize(1280, 720);
    Scene::SetBackground(Color { 20 });

    Scene::SetResizeMode(ResizeMode::Keep);

    const Array<std::pair<String, Array<String>>> menu_items = {
        { U"File", { U"\U000F0214 Open", U"\U000F05AD Exit" } },
    };

    SimpleMenuBar menu_bar(menu_items);

    SideMenu side_menu;

    while (System::Update()) {
        side_menu.update();

        if (const auto item = menu_bar.update()) {
            // Exit
            if (item == MenuBarItemIndex { 0, 1 }) {
                System::Exit();
            }

            // Open
            if (item == MenuBarItemIndex { 0, 0 }) {
                if (not SheetManagerAddon::IsLoading()) {
                    const auto path = Dialog::OpenFile({ FileFilter::JSON() });
                    if (path) {
                        SheetManagerAddon::LoadAsync(*path);
                    }
                }
            }
        }

        const String time = FormatTime(SecondsF { SheetManagerAddon::PosSec() }, U"M:ss")
            + U" / " + FormatTime(SecondsF { SheetManagerAddon::LengthSec() }, U"M:ss");

        double progress = static_cast<double>(SheetManagerAddon::PosSec()) / SheetManagerAddon::LengthSec();

        const auto enabled = SheetManagerAddon::IsLoaded();
        const auto playing_label = enabled and SheetManagerAddon::IsPlaying() ? U"\U000F03E4" : U"\U000F040A";

        // play / pause
        if (SimpleGUI::Button(playing_label, Vec2 { 300, 685 }, 50, enabled)) {
            SheetManagerAddon::Toggle();
        }

        // stop
        if (SimpleGUI::Button(U"\U000F04DB", Vec2 { 350, 685 }, 50, enabled)) {
            SheetManagerAddon::Stop();
        }

        // seek bar
        if (SimpleGUI::Slider(time, progress, 0.0, 1.0, Vec2 { 400, 685 }, 100, 730, enabled)) {
            SheetManagerAddon::SeekTime(SheetManagerAddon::LengthSec() * progress);
        }

        // reload
        if (SimpleGUI::Button(U"\U000F0453", Vec2 { 1230, 685 }, 50, enabled)) {
            const auto path = SheetManagerAddon::GetMetadata().path;
            SheetManagerAddon::LoadAsync(path);
        }

        // Side menu
        side_menu.draw();

        menu_bar.draw();
    }
}
