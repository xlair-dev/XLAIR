#include "SheetViewer.hpp"

#include "addon/SheetManagerAddon.hpp"

bool SheetViewer::update() {
    // Update logic for the sheet viewer
    if (SheetManagerAddon::IsLoaded()) {
        // Update the sheet data, music, and other elements
        // This is where you would handle playback, seeking, etc.
    }

    return true;
}

void SheetViewer::draw() const {
    const auto width = Scene::Width() - UI::OffsetWidth;
    const auto height = Scene::Height();
    const Rect viewport_rect { Scene::Width() - width, 0, width, height};
    const ScopedViewport2D viewport { viewport_rect };

    {
        // draw lanes
        const auto center_x = width / 2.0;
        const auto offset_left = (width - UI::StageWidth) / 2.0;

        // left side
        Line { offset_left, 0, offset_left, height }.draw(1);

        // center lanes
        for (int32 i = 0; i < UI::LaneNum; ++i) {
            const auto x = offset_left + UI::SideLaneWidth + i * UI::LaneWidth;
            Line { x, 0, x, height }.draw(1);
        }

        // right side
        const auto offset_right = offset_left + UI::SideLaneWidth + UI::LaneNum * UI::LaneWidth;
        Line { offset_right, 0, offset_right, height }.draw(1);
    }
}
