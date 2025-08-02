#include <Siv3D.hpp> // Siv3D v0.6.16

#include "SheetManager.hpp"
#include "SideMenu.hpp"

#include "addon/NotificationAddon.hpp"
#include "addon/LoadingAnimationAddon.hpp"

void Main() {
    Addon::Register<NotificationAddon>(NotificationAddon::Name);
    Addon::Register<LoadingAnimationAddon>(LoadingAnimationAddon::Name);

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

    double pos = 0.0;

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

        const auto enabled = sheet_manager.isLoaded();
        const auto playing_label = enabled and AudioAsset(U"music").isPlaying() ? U"\U000F03E4" : U"\U000F040A";

        if (SimpleGUI::Button(playing_label, Vec2 { 300, 685 }, 50, enabled)) {
            //if (AudioAsset(U"music").isPlaying()) {
            //    AudioAsset(U"music").pause();
            //} else {
            //    AudioAsset(U"music").play();
            //}
        }
        SimpleGUI::Button(U"\U000F04DB", Vec2 { 350, 685 }, 50, enabled);
        SimpleGUI::Slider(pos, 0.0, 1.0, Vec2 { 400, 685 }, 830, enabled);
        SimpleGUI::Button(U"\U000F0453", Vec2 { 1230, 685 }, 50, enabled);

        // Side menu
        side_menu.draw(sheet_manager);

        menu_bar.draw();

        if (SimpleGUI::Button(U"information", Vec2 { 600, 80 }, 160)) {
            NotificationAddon::Show(U"information", NotificationAddon::Type::Information);
        }
    }
}
