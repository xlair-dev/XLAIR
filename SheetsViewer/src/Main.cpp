#include <Siv3D.hpp> // Siv3D v0.6.16

#include "SheetManager.hpp"
#include "SideMenu.hpp"

#include "addon/NotificationAddon.hpp"
#include "addon/LoadingAnimationAddon.hpp"

void Main() {
    Addon::Register<NotificationAddon>(NotificationAddon::Name);
    Addon::Register<LoadingAnimationAddon>(LoadingAnimationAddon::Name);

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

    SheetManager sheet_manager(U"viewer");

    SideMenu side_menu;

    while (System::Update()) {
        sheet_manager.update();
        side_menu.update(sheet_manager);

        if (const auto item = menu_bar.update()) {
            // Exit
            if (item == MenuBarItemIndex { 0, 1 }) {
                System::Exit();
            }

            // Open
            if (item == MenuBarItemIndex { 0, 0 }) {
                if (not sheet_manager.isLoading()) {
                    const auto path = Dialog::OpenFile({ FileFilter::JSON() });
                    if (path) {
                        sheet_manager.loadAsync(*path);
                    }
                }
            }
        }

        const String time = FormatTime(SecondsF { sheet_manager.posSec() }, U"M:ss")
            + U" / " + FormatTime(SecondsF { sheet_manager.lengthSec() }, U"M:ss");

        double progress = static_cast<double>(sheet_manager.posSec()) / sheet_manager.lengthSec();

        const auto enabled = sheet_manager.isLoaded();
        const auto playing_label = enabled and sheet_manager.isPlaying() ? U"\U000F03E4" : U"\U000F040A";

        // play / pause
        if (SimpleGUI::Button(playing_label, Vec2 { 300, 685 }, 50, enabled)) {
            if (sheet_manager.isPlaying()) {
                sheet_manager.pause();
            } else {
                sheet_manager.play();
            }
        }

        // stop
        if (SimpleGUI::Button(U"\U000F04DB", Vec2 { 350, 685 }, 50, enabled)) {
            sheet_manager.stop();
        }

        // seek bar
        if (SimpleGUI::Slider(time, progress, 0.0, 1.0, Vec2 { 400, 685 }, 100, 730, enabled)) {
            sheet_manager.seekTime(sheet_manager.lengthSec() * progress);
        }

        // reload
        if (SimpleGUI::Button(U"\U000F0453", Vec2 { 1230, 685 }, 50, enabled)) {
            const auto path = sheet_manager.getMetadata().path;
            sheet_manager.loadAsync(path);
        }

        // Side menu
        side_menu.draw(sheet_manager);

        menu_bar.draw();
    }
}
