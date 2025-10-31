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
    const Rect viewport_rect{ Scene::Width() - width, 0, width, height };
    const ScopedViewport2D viewport{ viewport_rect };

    const auto center_x = width / 2.0;
    const auto offset_left = (width - UI::StageWidth) / 2.0;
    {
        // draw lanes

        // center lanes
        RectF{ offset_left + 2 * UI::SideLaneWidth, 0, UI::LaneNum * UI::LaneWidth, height }.draw(Color(20));
        for (int32 i = 0; i <= UI::LaneNum; i++) {
            const auto x = offset_left + 2 * UI::SideLaneWidth + i * UI::LaneWidth;
            const auto line_color = (i % 2 == 0) ? ColorF(0.8) : ColorF(0.4);
            Line{ x, 0, x, height }.draw(1, line_color);
        }

        // left side
        RectF{ offset_left, 0, UI::SideLaneWidthOffset, height }.draw(ColorF(0.12));
        for (int32 i = 0; i <= UI::SideLaneNum; i++) {
            const auto x = offset_left + i * UI::SideLaneWidth;
            const auto line_width = (i % 2 == 0) ? 1.5 : 1.0;
            const auto line_color = (i % 2 == 0) ? ColorF(0.9) : ColorF(0.6);
            Line{ x, 0, x, height }.draw(line_width, line_color);
        }

        // right side
        const auto offset_right = offset_left + UI::SideLaneWidthOffset + UI::LaneNum * UI::LaneWidth;
        RectF{ offset_right, 0, UI::SideLaneWidthOffset, height }.draw(ColorF(0.12));
        for (int32 i = 0; i <= UI::SideLaneNum; i++) {
            const auto x = offset_right + i * UI::SideLaneWidth;
            const auto line_width = (i % 2 == 0) ? 1.5 : 1.0;
            const auto line_color = (i % 2 == 0) ? ColorF(0.9) : ColorF(0.6);
            Line{ x, 0, x, height }.draw(line_width, line_color);
        }
    }

    {
        // draw judgebar
        const auto judgebar_y = height - 120;
        Line{ offset_left, judgebar_y, offset_left + UI::StageWidth, judgebar_y }.draw(2, ColorF(1.0, 0, 0));
    }

    const auto selected_difficulty = SheetManagerAddon::GetSelectedDifficulty();
    const auto pos_sample = SheetManagerAddon::PosSample();

    if (selected_difficulty) {
        if (const auto sheet_data = SheetManagerAddon::GetSheetData(*selected_difficulty)) {
            if (sheet_data->valid) {
                // Draw the sheet data

                for (const auto& tap : sheet_data->notes.tap) {
                    const auto x = offset_left + 2 * UI::SideLaneWidth + tap.start_lane * UI::LaneWidth;
                    const auto note_width = tap.width * UI::LaneWidth;
                    const auto y = (height - 120) - static_cast<double>(tap.sample - pos_sample) * 0.01;
                    RectF{ x, y, note_width, 20.0 }.draw(ColorF(0.8, 0.2, 0.2));
                }

                for (const auto& xtap : sheet_data->notes.xtap) {
                    const auto x = offset_left + 2 * UI::SideLaneWidth + xtap.start_lane * UI::LaneWidth;
                    const auto note_width = xtap.width * UI::LaneWidth;
                    const auto y = (height - 120) - static_cast<double>(xtap.sample - pos_sample) * 0.01;
                    RectF{ x, y, note_width, 20.0 }.draw(ColorF(0.8, 0.8, 0.0));
                }

                for (const auto& flick : sheet_data->notes.flick) {
                    const auto x = offset_left + 2 * UI::SideLaneWidth + flick.start_lane * UI::LaneWidth;
                    const auto note_width = flick.width * UI::LaneWidth;
                    const auto y = (height - 120) - static_cast<double>(flick.sample - pos_sample) * 0.01;
                    RectF{ x, y, note_width, 20.0 }.draw(ColorF(0.2, 0.2, 0.8));
                }

                for (const auto& hold : sheet_data->notes.hold) {
                    for (size_t i = 1; i < hold.notes.size(); i++) {
                        const auto& pre = hold.notes[i - 1];
                        const auto& cur = hold.notes[i];

                        const auto pre_x = offset_left + 2 * UI::SideLaneWidth + pre.start_lane * UI::LaneWidth;
                        const auto cur_x = offset_left + 2 * UI::SideLaneWidth + cur.start_lane * UI::LaneWidth;

                        const auto pre_w = pre.width * UI::LaneWidth;
                        const auto cur_w = cur.width * UI::LaneWidth;

                        const auto pre_y = (height - 120) - static_cast<double>(pre.sample - pos_sample) * 0.01;
                        const auto cur_y = (height - 120) - static_cast<double>(cur.sample - pos_sample) * 0.01;

                        Quad{
                            Vec2{ pre_x, pre_y },
                            Vec2{ pre_x + pre_w, pre_y },
                            Vec2{ cur_x + cur_w, cur_y },
                            Vec2{ cur_x, cur_y },
                        }.draw(ColorF(0.2, 0.2, 0.8, 0.5));
                    }
                    for (const auto& judge : hold.judge) {
                        const auto x = offset_left + 2 * UI::SideLaneWidth + judge.start_lane * UI::LaneWidth;
                        const auto note_width = judge.width * UI::LaneWidth;
                        const auto y = (height - 120) - static_cast<double>(judge.sample - pos_sample) * 0.01;
                        RectF{ x, y, note_width, 10.0 }.draw(ColorF(0.8, 0.8, 0.8, 0.5));
                    }
                }
            }
        }
    }
}
