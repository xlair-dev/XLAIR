#include "SideMenu.hpp"

void SideMenu::draw(const SheetManager& sheet_manager) const {
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
