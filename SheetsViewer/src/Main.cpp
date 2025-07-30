#include <Siv3D.hpp> // Siv3D v0.6.16

#include "SheetManager.hpp"
#include "addon/NotificationAddon.hpp"

#include "SheetsAnalyzer.hpp"
#include "SUSAnalyzer/SUSAnalyzer.hpp"

void Main() {
    Addon::Register<NotificationAddon>(U"NotificationAddon");

    Window::SetStyle(WindowStyle::Sizable);
    Window::Resize(1280, 720);
    Scene::SetBackground(Color { 20 });

    Scene::SetResizeMode(ResizeMode::Keep);

    const Array<std::pair<String, Array<String>>> menu_items = {
        { U"File", { U"\U000F0214 Open", U"\U000F05AD Exit" } },
    };

    const Array<std::pair<String, String>> side_menu_items = {
        { U"\U000F0EFE", U"ID" },
        { U"\U000F0CB8", U"Title" },
        { U"\U000F04BA", U"Sort" },
        { U"\U000F0803", U"Artist" },
        { U"\U000F0770", U"Genre" },
        { U"\U000F0E2A", U"Music" },
        { U"\U000F021F", U"Jacket" },
        { U"\U000F0337", U"URL" },
        { U"\U000F05B7", U"Offset" },
        { U"\U000F07DA", U"BPM" },
    };

    const auto font = SimpleGUI::GetFont();

    SimpleMenuBar menu_bar(menu_items);

    SheetManager sheet_manager(U"viewr");

    double pos = 0.0;

    while (System::Update()) {
        sheet_manager.update();

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
        {
            const Rect viewport_rect { 0, 0, 300, Scene::Height() };
            const ScopedViewport2D viewport { viewport_rect };
            const auto& metadata = sheet_manager.getMetadata();
            const auto indexed_data = std::array {
                metadata.id,
                metadata.title,
                metadata.title_sort,
                metadata.artist,
                metadata.genre,
                FileSystem::RelativePath(metadata.music),
                FileSystem::RelativePath(metadata.jacket),
                metadata.url,
                Format(metadata.music_offset),
                Format(metadata.bpm),
            };

            viewport_rect.draw(Color { 50 });

            side_menu_items.each_index([&](size_t index, const auto& item) {
                const auto rect = Rect { 10, 50 + index * 27, 200, 25 };
                if (rect.leftClicked()) {
                    // Handle click on side menu item
                }
                const auto& [icon, text] = item;
                font(icon).draw(20, Arg::leftCenter = rect.pos, ColorF { 0.7 });
                font(text).draw(15, Arg::leftCenter = rect.pos.movedBy(25, 0), ColorF { 0.7 });

                if (sheet_manager.isLoaded() && not metadata.path.isEmpty()) {
                    font(indexed_data[index]).draw(15, Arg::leftCenter = rect.pos.movedBy(80, 0), ColorF { 0.95 });
                }
            });
        }

        if (sheet_manager.isLoading()) {
            Rect { 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF { 0, 0.5 });
        }

        menu_bar.draw();

        if (SimpleGUI::Button(U"information", Vec2 { 600, 80 }, 160)) {
            NotificationAddon::Show(U"information", NotificationAddon::Type::Information);
        }
    }
}
