#include "AnalysisRenderer.hpp"

#include <algorithm>
#include <cmath>

namespace xlair::sheets_viewer {
    namespace {
        constexpr double JudgeMargin = 72.0;
        constexpr double NoteHeight = 12.0;
        constexpr double SliderLaneOffset = 2.0;
        constexpr double PlayfieldLaneCount = 20.0;

        struct LaneRange {
            double start = 0.0;
            double width = 1.0;
        };

        [[nodiscard]]
        LaneRange SliderLaneRange(const sheets::LaneSpan lane) {
            return {
                .start = SliderLaneOffset + lane.start,
                .width = static_cast<double>(lane.width),
            };
        }

        [[nodiscard]]
        LaneRange SideButtonRange(const sheets::SideButton button) {
            switch (button) {
                case sheets::SideButton::LeftUpper:
                    return { .start = 0.0, .width = 1.0 };
                case sheets::SideButton::LeftLower:
                    return { .start = 1.0, .width = 1.0 };
                case sheets::SideButton::RightLower:
                    return { .start = 18.0, .width = 1.0 };
                case sheets::SideButton::RightUpper:
                    return { .start = 19.0, .width = 1.0 };
            }

            return {};
        }

        [[nodiscard]]
        double SampleY(
            const playfield::ChartProjection& projection,
            const sheets::TimelineIndex timeline,
            const s3d::int64 current_sample,
            const s3d::int64 sample,
            const double judge_y,
            const double pixels_per_second
        ) {
            const long double distance = projection.noteDistance(timeline, current_sample, sample);
            return judge_y - static_cast<double>(distance) * pixels_per_second;
        }

        [[nodiscard]]
        s3d::RectF LaneRect(
            const LaneRange range,
            const double stage_left,
            const double lane_width,
            const double y,
            const double height
        ) {
            return {
                stage_left + range.start * lane_width,
                y,
                range.width * lane_width,
                height,
            };
        }

        template <class Point>
        [[nodiscard]]
        sheets::TimelineIndex TimelineAtSample(const s3d::Array<Point>& points, const s3d::int64 sample) {
            const auto point = std::upper_bound(
                points.begin(),
                points.end(),
                sample,
                [](const s3d::int64 value, const Point& candidate) {
                    return value < candidate.sample;
                }
            );
            return (point == points.begin() ? points.front() : *std::prev(point)).timeline;
        }

        [[nodiscard]]
        bool IsVisible(const double y, const double height) {
            return (-NoteHeight <= y && y <= (height + NoteHeight));
        }

        [[nodiscard]]
        s3d::ColorF SliderNoteColor(const sheets::SliderNoteKind kind) {
            switch (kind) {
                case sheets::SliderNoteKind::Tap:
                    return s3d::ColorF{ 0.95, 0.28, 0.32 };
                case sheets::SliderNoteKind::XTap:
                    return s3d::ColorF{ 1.0, 0.84, 0.22 };
                case sheets::SliderNoteKind::Flick:
                    return s3d::ColorF{ 0.30, 0.62, 1.0 };
            }
            return s3d::Palette::White;
        }
    }

    void AnalysisRenderer::draw(
        const sheets::Chart& chart,
        const playfield::ChartProjection& projection,
        const s3d::int64 current_sample,
        const s3d::Rect& viewport,
        const double pixels_per_second
    ) const {
        const s3d::ScopedViewport2D scoped_viewport{ viewport };
        const double width = viewport.w;
        const double height = viewport.h;
        const double lane_width = std::min(36.0, (width - 32.0) / PlayfieldLaneCount);
        const double stage_width = lane_width * PlayfieldLaneCount;
        const double stage_left = (width - stage_width) * 0.5;
        const double judge_y = height - JudgeMargin;

        s3d::RectF{ 0, 0, width, height }.draw(s3d::ColorF{ 0.08, 0.09, 0.12 });
        LaneRect({ .start = 0, .width = 2 }, stage_left, lane_width, 0, height).draw(s3d::ColorF{ 0.13, 0.14, 0.18 });
        LaneRect({ .start = 2, .width = 16 }, stage_left, lane_width, 0, height).draw(s3d::ColorF{ 0.09, 0.10, 0.13 });
        LaneRect({ .start = 18, .width = 2 }, stage_left, lane_width, 0, height).draw(s3d::ColorF{ 0.13, 0.14, 0.18 });

        for (s3d::int32 lane = 0; lane <= static_cast<s3d::int32>(PlayfieldLaneCount); ++lane) {
            const double x = stage_left + lane * lane_width;
            const bool edge = (lane == 0 || lane == 2 || lane == 18 || lane == 20);
            s3d::Line{ x, 0, x, height }.draw(edge ? 1.5 : 1.0, s3d::ColorF{ 0.55, edge ? 0.75 : 0.35 });
        }

        // Hold bodies are drawn first so their anchors and short notes stay readable.
        for (const auto& hold : chart.slider_holds) {
            for (std::size_t index = 1; index < hold.points.size(); ++index) {
                const auto& previous = hold.points[index - 1];
                const auto& current = hold.points[index];
                const auto previous_range = SliderLaneRange(previous.lane);
                const auto current_range = SliderLaneRange(current.lane);
                const double previous_y =
                    SampleY(projection, previous.timeline, current_sample, previous.sample, judge_y, pixels_per_second);
                const double current_y =
                    SampleY(projection, current.timeline, current_sample, current.sample, judge_y, pixels_per_second);
                const double previous_x = stage_left + previous_range.start * lane_width;
                const double current_x = stage_left + current_range.start * lane_width;
                const double previous_width = previous_range.width * lane_width;
                const double current_width = current_range.width * lane_width;

                s3d::Quad{
                    s3d::Vec2{ previous_x, previous_y },
                    s3d::Vec2{ previous_x + previous_width, previous_y },
                    s3d::Vec2{ current_x + current_width, current_y },
                    s3d::Vec2{ current_x, current_y },
                }
                    .draw(s3d::ColorF{ 0.20, 0.58, 0.95, 0.42 });
            }

            for (const auto& judge : hold.judge_points) {
                if (hold.points.isEmpty()) {
                    continue;
                }
                const auto timeline = TimelineAtSample(hold.points, judge.sample);
                const double y =
                    SampleY(projection, timeline, current_sample, judge.sample, judge_y, pixels_per_second);
                if (IsVisible(y, height)) {
                    LaneRect(SliderLaneRange(judge.lane), stage_left, lane_width, y - 2, 4)
                        .draw(s3d::ColorF{ 0.75, 0.88, 1.0, 0.65 });
                }
            }

            for (const auto& point : hold.points) {
                const double y =
                    SampleY(projection, point.timeline, current_sample, point.sample, judge_y, pixels_per_second);
                if (!IsVisible(y, height)) {
                    continue;
                }

                const auto rect = LaneRect(SliderLaneRange(point.lane), stage_left, lane_width, y - 4, 8);
                if (point.kind == sheets::SliderHoldPointKind::Invisible) {
                    rect.drawFrame(1, s3d::ColorF{ 0.45, 0.75, 1.0, 0.8 });
                } else {
                    rect.draw(s3d::ColorF{ 0.28, 0.72, 1.0 });
                }
            }
        }

        for (const auto& hold : chart.side_holds) {
            const auto range = SideButtonRange(hold.button);
            for (std::size_t index = 1; index < hold.points.size(); ++index) {
                const auto& previous = hold.points[index - 1];
                const auto& current = hold.points[index];
                const double previous_y =
                    SampleY(projection, previous.timeline, current_sample, previous.sample, judge_y, pixels_per_second);
                const double current_y =
                    SampleY(projection, current.timeline, current_sample, current.sample, judge_y, pixels_per_second);
                const double top = std::min(previous_y, current_y);
                LaneRect(range, stage_left, lane_width, top, std::abs(current_y - previous_y))
                    .draw(s3d::ColorF{ 0.72, 0.28, 0.92, 0.45 });
            }

            for (const auto sample : hold.judge_samples) {
                if (hold.points.isEmpty()) {
                    continue;
                }
                const auto timeline = TimelineAtSample(hold.points, sample);
                const double y = SampleY(projection, timeline, current_sample, sample, judge_y, pixels_per_second);
                if (IsVisible(y, height)) {
                    LaneRect(range, stage_left, lane_width, y - 2, 4).draw(s3d::ColorF{ 0.92, 0.70, 1.0, 0.75 });
                }
            }
        }

        for (const auto& note : chart.slider_notes) {
            const double y =
                SampleY(projection, note.timeline, current_sample, note.sample, judge_y, pixels_per_second);
            if (IsVisible(y, height)) {
                LaneRect(SliderLaneRange(note.lane), stage_left, lane_width, y - NoteHeight * 0.5, NoteHeight)
                    .rounded(2)
                    .draw(SliderNoteColor(note.kind));
            }
        }

        for (const auto& note : chart.side_notes) {
            const double y =
                SampleY(projection, note.timeline, current_sample, note.sample, judge_y, pixels_per_second);
            if (IsVisible(y, height)) {
                LaneRect(SideButtonRange(note.button), stage_left, lane_width, y - NoteHeight * 0.5, NoteHeight)
                    .rounded(2)
                    .draw(s3d::ColorF{ 0.92, 0.38, 0.92 });
            }
        }

        s3d::Line{ stage_left, judge_y, stage_left + stage_width, judge_y }.draw(3, s3d::ColorF{ 1.0, 0.30, 0.32 });
    }
}
