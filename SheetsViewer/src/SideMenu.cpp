#include "SideMenu.hpp"
#include "addon/NotificationAddon.hpp"

void SideMenu::update(const SheetManager& sheet_manager) {
    const auto current_state = sheet_manager.getState();
    const auto& metadata = sheet_manager.getMetadata();

    if (sheet_manager.isLoaded()) {
        // update metadata
        if (m_previous_state != current_state) {
            m_indexed_metadata = std::array {
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
        }

        const auto offset = UI::ItemOffset + SideMenuItems.size() * UI::ItemHeight;
        for (const auto& [index, difficulty] : Indexed(metadata.difficulties)) {
            const auto is_valid = sheet_manager.isLoaded() and not difficulty.src.isEmpty();
            const auto rect = Rect { UI::Margin, offset + UI::ItemDifficultyOffset + (index + 1) * UI::ItemDifficultyHeight, UI::ItemWidth, UI::ItemDifficultyHeight };

            if (is_valid and rect.leftClicked()) {
                m_selected_index = index;
                NotificationAddon::Show(U"Selected difficulty: {}"_fmt(index), NotificationAddon::Type::Information);
            }
        }
    } else {
        m_selected_index = none;
    }

    m_previous_state = current_state;
}

void SideMenu::draw(const SheetManager& sheet_manager) const {
    const auto& font = SimpleGUI::GetFont();
    const auto& metadata = sheet_manager.getMetadata();

    const Rect viewport_rect { 0, 0, UI::Width, Scene::Height() };
    const ScopedViewport2D viewport { viewport_rect };

    viewport_rect.draw(Color { 50 });

    for (const auto& [index, item] : IndexedRef(SideMenuItems)) {
        const auto pos = Vec2 { UI::Margin, UI::ItemOffset + index * UI::ItemHeight };
        const auto& [icon, text] = item;

        font(icon).draw(UI::ItemIconSize, Arg::leftCenter = pos, ColorF { 0.7 });
        font(text).draw(UI::ItemLabelSize, Arg::leftCenter = pos.movedBy(UI::ItemLabelOffset, 0), ColorF { 0.7 });

        if (sheet_manager.isLoaded() and not metadata.path.isEmpty()) {
            font(m_indexed_metadata[index]).draw(UI::ItemTextSize, Arg::leftCenter = pos.movedBy(UI::ItemTextOffset, 0), ColorF { 0.95 });
        }
    }

    const auto offset = UI::ItemOffset + SideMenuItems.size() * UI::ItemHeight;

    // difficulties

    Line { UI::Margin, offset, UI::Width - UI::Margin, offset }.draw(ColorF { 0.7 });

    font(U"\U000F041F").draw(UI::ItemIconSize, Arg::leftCenter = Vec2 { UI::Margin, offset + UI::ItemIconSize }, ColorF { 0.7 });
    font(U"difficulties").draw(UI::ItemLabelSize, Arg::leftCenter = Vec2 { UI::Margin + UI::ItemLabelOffset, offset + UI::ItemIconSize }, ColorF { 0.7 });

    for (const auto& [index, difficulty] : Indexed(metadata.difficulties)) {
        const auto is_valid = sheet_manager.isLoaded() and not difficulty.src.isEmpty();
        const auto rect = Rect { UI::Margin, offset + UI::ItemDifficultyOffset + (index + 1) * UI::ItemDifficultyHeight, UI::ItemWidth, UI::ItemDifficultyHeight };
        const auto left_center = rect.leftCenter();
        const auto color = is_valid ? ColorF { 0.95 } : ColorF { 0.5 };

        if (is_valid and rect.mouseOver()) {
            Cursor::RequestStyle(CursorStyle::Hand);
            rect.stretched(UI::Margin / 2, 0).rounded(2).draw(ColorF { 0.4 });
        }

        const auto selected = m_selected_index and *m_selected_index == index;

        if (selected) {
            rect.stretched(UI::Margin / 2, 0).rounded(2).draw(ColorF { 0.3 });
        }

        font(selected ? U"\U000F043E" : U"\U000F043D").draw(UI::ItemLabelSize, Arg::leftCenter = left_center, color);
        font(U"ID: {}"_fmt(index)).draw(UI::ItemLabelSize, Arg::leftCenter = left_center.movedBy(UI::ItemLabelOffset, 0), color);

        if (is_valid) {
            font(U"\U000F041F Level: {}"_fmt(difficulty.level)).draw(UI::ItemTextSize, Arg::leftCenter = left_center.movedBy(UI::ItemDifficultyTextOffset, 0), ColorF { 0.95 });
        }
    }

    // jacket
    if (const auto jacket = sheet_manager.getJacket()) {
        jacket->fitted(UI::JacketSize).draw(Arg::bottomLeft = Vec2 { UI::Margin, Scene::Height() - UI::Margin });
    }
}
