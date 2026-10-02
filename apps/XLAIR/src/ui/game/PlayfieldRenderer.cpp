#include "PlayfieldRenderer.hpp"

#include <cmath>

namespace xlair::ui::game {
    namespace {
        // Temporary flat layout for validating chart timing and playback.
        constexpr double LaneCount = 20.0;
        constexpr double SliderLaneOffset = 2.0;
        constexpr double JudgeMargin = 90.0;
        constexpr double NoteHeight = 14.0;

        struct LaneRange {
            double start = 0.0;
            double width = 1.0;
        };

        [[nodiscard]]
        LaneRange SliderRange(const sheets::LaneSpan lane) {
            return { .start = SliderLaneOffset + lane.start, .width = static_cast<double>(lane.width) };
        }

        [[nodiscard]]
        LaneRange SideRange(const sheets::SideButton button) {
            switch (button) {
                case sheets::SideButton::LeftUpper:
                    return { .start = 0.0 };
                case sheets::SideButton::LeftLower:
                    return { .start = 1.0 };
                case sheets::SideButton::RightLower:
                    return { .start = 18.0 };
                case sheets::SideButton::RightUpper:
                    return { .start = 19.0 };
            }
            return {};
        }

        [[nodiscard]]
        double NoteY(
            const playfield::ChartProjection& projection,
            const sheets::TimelineIndex timeline,
            const int64 current_sample,
            const int64 note_sample,
            const double judge_y,
            const double pixels_per_second
        ) {
            return judge_y - static_cast<double>(projection.noteDistance(timeline, current_sample, note_sample)) *
                                 pixels_per_second;
        }

        [[nodiscard]]
        RectF LaneRect(
            const LaneRange range,
            const double stage_left,
            const double lane_width,
            const double y,
            const double height
        ) {
            return { stage_left + range.start * lane_width, y, range.width * lane_width, height };
        }

        [[nodiscard]]
        bool Visible(const double y, const double height) {
            return std::isfinite(y) && (-NoteHeight <= y) && (y <= height + NoteHeight);
        }

        [[nodiscard]]
        ColorF SliderColor(const sheets::SliderNoteKind kind) {
            switch (kind) {
                case sheets::SliderNoteKind::Tap:
                    return ColorF{ 0.20, 0.86, 0.92 };
                case sheets::SliderNoteKind::XTap:
                    return ColorF{ 1.0, 0.81, 0.24 };
                case sheets::SliderNoteKind::Flick:
                    return ColorF{ 0.35, 0.57, 1.0 };
            }
            return Palette::White;
        }
    }

    void PlayfieldRenderer::draw(
        const sheets::Chart& chart,
        const playfield::ChartProjection& projection,
        const int64 current_sample,
        const Rect& viewport,
        const double pixels_per_second
    ) const {
        const ScopedViewport2D scoped_viewport{ viewport };
        const double width = viewport.w;
        const double height = viewport.h;
        const double lane_width = Min(44.0, (width - 32.0) / LaneCount);
        const double stage_width = lane_width * LaneCount;
        const double stage_left = (width - stage_width) * 0.5;
        const double judge_y = height - JudgeMargin;

        RectF{ 0, 0, width, height }.draw(ColorF{ 0.05, 0.08, 0.13 });
        LaneRect({ .start = 0.0, .width = 2.0 }, stage_left, lane_width, 0, height).draw(ColorF{ 0.12, 0.13, 0.20 });
        LaneRect({ .start = 2.0, .width = 16.0 }, stage_left, lane_width, 0, height).draw(ColorF{ 0.08, 0.11, 0.17 });
        LaneRect({ .start = 18.0, .width = 2.0 }, stage_left, lane_width, 0, height).draw(ColorF{ 0.12, 0.13, 0.20 });

        for (int32 lane = 0; lane <= static_cast<int32>(LaneCount); ++lane) {
            const bool boundary = (lane == 0 || lane == 2 || lane == 18 || lane == 20);
            const bool group = (lane >= 2 && lane <= 18 && (lane - 2) % 4 == 0);
            const double x = stage_left + lane * lane_width;
            Line{ x, 0, x, height }.draw(boundary || group ? 2.0 : 1.0, ColorF{ 0.65, 0.77, 0.92, 0.35 });
        }

        // Draw hold bodies underneath their visible points and short notes.
        for (const auto& hold : chart.slider_holds) {
            for (std::size_t index = 1; index < hold.points.size(); ++index) {
                const auto& previous = hold.points[index - 1];
                const auto& current = hold.points[index];
                const double previous_y =
                    NoteY(projection, previous.timeline, current_sample, previous.sample, judge_y, pixels_per_second);
                const double current_y =
                    NoteY(projection, current.timeline, current_sample, current.sample, judge_y, pixels_per_second);
                if (!std::isfinite(previous_y) || !std::isfinite(current_y) || (previous_y < 0 && current_y < 0) ||
                    (previous_y > height && current_y > height)) {
                    continue;
                }

                const auto before = SliderRange(previous.lane);
                const auto after = SliderRange(current.lane);
                const double previous_x = stage_left + before.start * lane_width;
                const double current_x = stage_left + after.start * lane_width;
                Quad{
                    Vec2{ previous_x, previous_y },
                    Vec2{ previous_x + before.width * lane_width, previous_y },
                    Vec2{ current_x + after.width * lane_width, current_y },
                    Vec2{ current_x, current_y },
                }
                    .draw(ColorF{ 0.24, 0.62, 1.0, 0.55 });
            }

            for (const auto& point : hold.points) {
                if (point.kind == sheets::SliderHoldPointKind::Invisible) {
                    continue;
                }
                const double y =
                    NoteY(projection, point.timeline, current_sample, point.sample, judge_y, pixels_per_second);
                if (Visible(y, height)) {
                    LaneRect(SliderRange(point.lane), stage_left, lane_width, y - 5.0, 10.0)
                        .rounded(2.0)
                        .draw(ColorF{ 0.42, 0.80, 1.0 });
                }
            }
        }

        for (const auto& hold : chart.side_holds) {
            const auto range = SideRange(hold.button);
            for (std::size_t index = 1; index < hold.points.size(); ++index) {
                const auto& previous = hold.points[index - 1];
                const auto& current = hold.points[index];
                const double previous_y =
                    NoteY(projection, previous.timeline, current_sample, previous.sample, judge_y, pixels_per_second);
                const double current_y =
                    NoteY(projection, current.timeline, current_sample, current.sample, judge_y, pixels_per_second);
                if (!std::isfinite(previous_y) || !std::isfinite(current_y) || (previous_y < 0 && current_y < 0) ||
                    (previous_y > height && current_y > height)) {
                    continue;
                }
                const double top = Min(previous_y, current_y);
                LaneRect(range, stage_left, lane_width, top, Abs(current_y - previous_y))
                    .draw(ColorF{ 0.80, 0.30, 0.82, 0.55 });
            }
            for (const auto& point : hold.points) {
                if (point.kind == sheets::SideHoldPointKind::Relay) {
                    continue;
                }
                const double y =
                    NoteY(projection, point.timeline, current_sample, point.sample, judge_y, pixels_per_second);
                if (Visible(y, height)) {
                    LaneRect(range, stage_left, lane_width, y - 5.0, 10.0)
                        .rounded(2.0)
                        .draw(ColorF{ 0.95, 0.55, 0.93 });
                }
            }
        }

        for (const auto& note : chart.slider_notes) {
            const double y = NoteY(projection, note.timeline, current_sample, note.sample, judge_y, pixels_per_second);
            if (Visible(y, height)) {
                LaneRect(SliderRange(note.lane), stage_left, lane_width, y - NoteHeight * 0.5, NoteHeight)
                    .rounded(2.0)
                    .draw(SliderColor(note.kind));
            }
        }
        for (const auto& note : chart.side_notes) {
            const double y = NoteY(projection, note.timeline, current_sample, note.sample, judge_y, pixels_per_second);
            if (Visible(y, height)) {
                LaneRect(SideRange(note.button), stage_left, lane_width, y - NoteHeight * 0.5, NoteHeight)
                    .rounded(2.0)
                    .draw(ColorF{ 0.95, 0.45, 0.92 });
            }
        }

        Line{ stage_left, judge_y, stage_left + stage_width, judge_y }.draw(4.0, ColorF{ 1.0, 0.35, 0.42 });
    }
}
