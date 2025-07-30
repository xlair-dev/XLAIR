#include <Siv3D.hpp> // Siv3D v0.6.16

#include "SheetManager.hpp"
#include "addon/NotificationAddon.hpp"
#include "addon/LoadingAnimationAddon.hpp"

#include "SheetsAnalyzer.hpp"
#include "SUSAnalyzer/SUSAnalyzer.hpp"

class SideMenu {
public:
    SideMenu() = default;
    ~SideMenu() = default;

    void draw(const SheetManager& sheet_manager) const {
        const auto& font = SimpleGUI::GetFont();
        const auto& metadata = sheet_manager.getMetadata();

        const Rect viewport_rect { 0, 0, 300, Scene::Height() };
        const ScopedViewport2D viewport { viewport_rect };

        viewport_rect.draw(Color { 50 });

        const auto indexed_metadata = std::array {
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

        for (const auto& [index, item] : IndexedRef(SideMenuItems)) {
            const auto rect = Rect { 10, 50 + index * 27, 200, 25 };
            const auto& [icon, text] = item;

            font(icon).draw(20, Arg::leftCenter = rect.pos, ColorF { 0.7 });
            font(text).draw(15, Arg::leftCenter = rect.pos.movedBy(25, 0), ColorF { 0.7 });

            if (sheet_manager.isLoaded() and not metadata.path.isEmpty()) {
                font(indexed_metadata[index]).draw(15, Arg::leftCenter = rect.pos.movedBy(80, 0), ColorF { 0.95 });
            }
        }
    }

private:
    static constexpr auto SideMenuItems = std::array {
        std::pair{ U"\U000F0EFE", U"ID" },
        std::pair{ U"\U000F0CB8", U"Title" },
        std::pair{ U"\U000F04BA", U"Sort" },
        std::pair{ U"\U000F0803", U"Artist" },
        std::pair{ U"\U000F0770", U"Genre" },
        std::pair{ U"\U000F0E2A", U"Music" },
        std::pair{ U"\U000F021F", U"Jacket" },
        std::pair{ U"\U000F0337", U"URL" },
        std::pair{ U"\U000F05B7", U"Offset" },
        std::pair{ U"\U000F07DA", U"BPM" },
    };
};

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
