#include "PlayfieldRenderer3D.hpp"

#include "core/judgement/JudgeEngine.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/primitives/Sparkle.hpp"

#include <SheetsAnalyzer/SliderHoldPath.hpp>

#include <algorithm>
#include <cmath>
#include <span>

namespace xlair::ui::game {
    namespace {
        constexpr Size FieldSurfaceSize{ 1000, 4000 };
        constexpr Size SideNoteSurfaceSize{ 512, 64 };
        constexpr double NotePlateHeight = 32.0;
        constexpr double NoteVisibilityMargin = 64.0;
        constexpr double SourceJudgeY = FieldSurfaceSize.y - 100.0;
        constexpr double FieldNear = PlayfieldRenderer3D::FieldNear;
        constexpr double FieldFar = PlayfieldRenderer3D::FieldFar;
        constexpr double FieldWidth = PlayfieldRenderer3D::FieldWidth;
        constexpr double FieldLength = FieldFar - FieldNear;
        constexpr double FieldCenter = (FieldNear + FieldFar) * 0.5;
        constexpr double StageLeft = FieldWidth * 32.0 / FieldSurfaceSize.x;
        constexpr double LaneWidth = (FieldWidth - StageLeft * 2.0) / 16.0;
        constexpr double LowerNear = FieldNear - 8.0;
        constexpr double LowerLength = FieldFar - LowerNear;
        constexpr double LowerCenter = (LowerNear + FieldFar) * 0.5;
        constexpr double LowerWidth = 3.0;
        constexpr double SideLowerAngle = 55_deg;
        const double SideLowerExtent = LowerWidth * Math::Cos(SideLowerAngle);
        const double SideLowerRise = LowerWidth * Math::Sin(SideLowerAngle);
        const double SideLowerX = FieldWidth * 0.5 + SideLowerExtent * 0.5;
        const double SideLowerY = SideLowerRise * 0.5;
        constexpr double SideUpperHeight = 3.0;
        const double SideUpperX = FieldWidth * 0.5 + SideLowerExtent;
        const double SideUpperY = SideLowerRise + SideUpperHeight * 0.5;
        constexpr double SideNoteDepthSize = FieldLength * NotePlateHeight / FieldSurfaceSize.y * 1.5;
        // Begin the fade gradually beyond the combo area, with the same world-z range for all 3D geometry.
        constexpr double FadeStart = FieldNear + FieldLength * 0.3;
        constexpr double FadeStrength = 0.8;
        const ColorF BackgroundColor{ U"#F2F3F5" };
        const ColorF SideUpperColor{ U"#D5F0FB" };
        const ColorF SideLowerColor{ BackgroundColor };
        const ColorF SideUpperNoteLightColor{ U"#504160" };
        const ColorF SideUpperNoteDarkColor{ U"#20172E" };
        const ColorF SideUpperNoteOutlineColor{ U"#100C19" };
        const ColorF SideLowerNoteLightColor{ U"#F060D8" };
        const ColorF SideLowerNoteDarkColor{ U"#BC269F" };
        const ColorF SideLowerNoteOutlineColor{ U"#661953" };
        // At 1.0x, map 100 px/s in the 880 px reference viewport to the 3D field length.
        constexpr double WorldUnitsPerSecond = FieldLength * 100.0 / 880.0;

        [[nodiscard]]
        Quaternion SideSlope(const double side) {
            return Quaternion::RotateZ(side < 0.0 ? 180_deg - SideLowerAngle : SideLowerAngle);
        }

        [[nodiscard]]
        Vec3 SideLowerPosition(const double side, const double z, const double offset) {
            return {
                side * (SideLowerX - offset * SideLowerRise / LowerWidth),
                SideLowerY + offset * SideLowerExtent / LowerWidth,
                z,
            };
        }

        [[nodiscard]]
        double WorldZ(
            const playfield::ChartProjection::Frame& projection,
            const sheets::TimelineIndex timeline,
            const int64 note_sample,
            const double note_speed
        ) {
            // Play options scale the visual distance without changing the chart's sample clock.
            return static_cast<double>(projection.noteDistance(timeline, note_sample)) * WorldUnitsPerSecond *
                   note_speed;
        }

        void UpdateMeasureLineDepths(
            const sheets::Chart& chart,
            const playfield::ChartProjection::Frame& projection,
            const double note_speed,
            Array<double>& depths
        ) {
            depths.clear();
            for (const auto& line : chart.measure_lines) {
                const double z = WorldZ(projection, chart.main_timeline, line.sample, note_speed);
                if (std::isfinite(z) && LowerNear <= z && z <= FieldFar) {
                    depths.push_back(z);
                }
            }
        }

        template <class Point, class DrawSection>
        void ForEachHoldSection(
            const playfield::ChartProjection::Frame& projection,
            const double note_speed,
            const Point& before,
            const Point& after,
            Array<int64>& samples,
            DrawSection&& draw_section
        ) {
            if (before.sample >= after.sample) {
                return;
            }

            projection.noteDistanceBreakpoints(before.timeline, before.sample, after.sample, samples);
            samples.push_back(after.sample);

            int64 previous_sample = before.sample;
            double previous_z = WorldZ(projection, before.timeline, previous_sample, note_speed);
            for (const int64 sample : samples) {
                // The preceding anchor owns the interval's scroll timeline. The final
                // endpoint keeps its own timeline, matching the anchor rendering.
                const auto timeline = sample == after.sample ? after.timeline : before.timeline;
                const double z = WorldZ(projection, timeline, sample, note_speed);
                if (std::isfinite(previous_z) && std::isfinite(z)) {
                    draw_section(previous_sample, previous_z, sample, z);
                }
                previous_sample = sample;
                previous_z = z;
            }
        }

        void DrawRoundedNotePlate(
            const Vec2& left_center,
            const double width,
            const double height,
            const ColorF& light,
            const ColorF& dark,
            const bool draw_overlay = false,
            const double shadow_blur = 30.0,
            const double shadow_offset = 10.0
        ) {
            // Rotate the vertical RoundRect gradient so it runs across the note's depth.
            const Transformer2D transform{ Mat3x2::Rotate(-90_deg, left_center) };
            const RoundRect plate =
                RoundRect{ left_center.movedBy(-height * 0.5, 0), height, width, 5.0 }.stretched(0, -5);
            plate.drawShadow(Vec2{ -shadow_offset, 0 }, shadow_blur, 0.0, dark.withA(0.25))
                .draw(Arg::top = light, Arg::bottom = dark);
            if (draw_overlay) {
                plate.movedBy(10, 0).stretched(0, -5).drawFrame(2.0, Arg::top = dark, Arg::bottom = light);
            }
        }

        template <class StateAt>
        [[nodiscard]]
        bool LastHoldJudgementHit(const std::size_t count, StateAt&& state_at) {
            if (count < 2 || state_at(count - 1) != core::judgement::NoteState::Pending) {
                return false;
            }
            for (std::size_t index = count - 1; index > 0; --index) {
                const auto state = state_at(index - 1);
                if (state != core::judgement::NoteState::Pending) {
                    return state == core::judgement::NoteState::Hit;
                }
            }
            return false;
        }

        [[nodiscard]]
        bool PressedOnSliderHoldPath(
            const s3d::Array<sheets::SliderHoldPoint>& points,
            const int64 sample,
            const uint32 slider_pressed
        ) {
            if (slider_pressed == 0) {
                return false;
            }
            const auto range = sheets::EvaluateSliderHoldLane(points, sample);
            if (!range) {
                return false;
            }
            for (int32 lane = range->start; lane < range->start + range->width; ++lane) {
                if ((slider_pressed & (uint32{ 3 } << (lane * 2))) != 0) {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]]
        bool SliderHoldActive(
            const sheets::SliderHold& hold,
            const std::size_t hold_index,
            const int64 current_sample,
            const uint32 slider_pressed,
            const core::judgement::JudgeEngine* judge_engine
        ) {
            if (!judge_engine || hold.points.size() < 2 || hold.judge_points.size() < 2 ||
                current_sample < hold.points.front().sample || current_sample >= hold.points.back().sample) {
                return false;
            }
            return LastHoldJudgementHit(
                       hold.judge_points.size(),
                       [judge_engine, hold_index](const std::size_t index) {
                           return judge_engine->sliderHoldPointState(hold_index, index);
                       }
                   ) &&
                   PressedOnSliderHoldPath(hold.points, current_sample, slider_pressed);
        }

        [[nodiscard]]
        bool SideHoldActive(
            const sheets::SideHold& hold,
            const std::size_t hold_index,
            const int64 current_sample,
            const uint8 side_pressed,
            const core::judgement::JudgeEngine* judge_engine
        ) {
            if (!judge_engine || hold.points.size() < 2 || hold.judge_samples.size() < 2 ||
                current_sample < hold.points.front().sample || current_sample >= hold.points.back().sample ||
                (side_pressed & (uint8{ 1 } << static_cast<uint8>(hold.button))) == 0) {
                return false;
            }
            return LastHoldJudgementHit(hold.judge_samples.size(), [judge_engine, hold_index](const std::size_t index) {
                return judge_engine->sideHoldPointState(hold_index, index);
            });
        }

        void DrawSliderHoldBodies(
            const sheets::Chart& chart,
            const playfield::ChartProjection::Frame& projection,
            const int64 current_sample,
            const double note_speed,
            const core::judgement::JudgeEngine* judge_engine,
            const uint32 slider_pressed,
            Array<int64>& section_samples
        ) {
            constexpr double SourceWidth = FieldSurfaceSize.x;
            constexpr double SourceHeight = FieldSurfaceSize.y;
            constexpr double SourceStageLeft = 32.0;
            constexpr double SourceLaneWidth = (SourceWidth - SourceStageLeft * 2.0) / 16.0;
            const ColorF start_color = ColorF{ U"#EC43F240" }.removeSRGBCurve();
            const ColorF end_color = ColorF{ U"#43A4F250" }.removeSRGBCurve();
            const ColorF center_color = ColorF{ U"#43A4F2" }.removeSRGBCurve();
            struct Edge {
                double left = 0.0;
                double right = 0.0;
                double y = 0.0;
                ColorF color;
            };

            Graphics2D::SetScissorRect(Rect{ 0, 0, FieldSurfaceSize.x, static_cast<int32>(SourceJudgeY) });
            RasterizerState clipped = RasterizerState::Default2D;
            clipped.scissorEnable = true;

            for (std::size_t hold_index = 0; hold_index < chart.slider_holds.size(); ++hold_index) {
                const auto& hold = chart.slider_holds[hold_index];
                if (hold.points.size() < 2) {
                    continue;
                }
                if (judge_engine && !hold.judge_points.isEmpty() &&
                    judge_engine->sliderHoldPointState(hold_index, hold.judge_points.size() - 1) !=
                        core::judgement::NoteState::Pending &&
                    current_sample >= hold.judge_points.back().sample) {
                    continue;
                }
                // A held body terminates at the judgement bar instead of scrolling below it.
                const ScopedRenderStates2D rasterizer{
                    SliderHoldActive(hold, hold_index, current_sample, slider_pressed, judge_engine)
                        ? clipped
                        : RasterizerState::Default2D
                };
                const int64 first_sample = hold.points.front().sample;
                const int64 last_sample = hold.points.back().sample;
                const double duration = static_cast<double>(last_sample) - static_cast<double>(first_sample);
                for (std::size_t index = 1; index < hold.points.size(); ++index) {
                    const auto& before = hold.points[index - 1];
                    const auto& after = hold.points[index];
                    const auto edge_at = [&](const int64 sample, const double z) {
                        const double ratio = static_cast<double>(
                            (static_cast<long double>(sample) - before.sample) /
                            (static_cast<long double>(after.sample) - before.sample)
                        );
                        const double left = SourceStageLeft +
                                            Math::Lerp(
                                                static_cast<double>(before.lane.start),
                                                static_cast<double>(after.lane.start),
                                                ratio
                                            ) * SourceLaneWidth +
                                            5.0;
                        const double width = Math::Lerp(
                                                 static_cast<double>(before.lane.width),
                                                 static_cast<double>(after.lane.width),
                                                 ratio
                                             ) * SourceLaneWidth -
                                             10.0;
                        const double color_t =
                            duration > 0.0 ? Clamp((static_cast<double>(sample) - first_sample) / duration, 0.0, 1.0)
                                           : 0.0;
                        return Edge{
                            .left = left,
                            .right = left + width,
                            .y = (FieldFar - z) * SourceHeight / FieldLength,
                            .color = Math::Lerp(start_color, end_color, color_t),
                        };
                    };
                    ForEachHoldSection(
                        projection,
                        note_speed,
                        before,
                        after,
                        section_samples,
                        [&](const int64 section_start,
                            const double before_z,
                            const int64 section_end,
                            const double after_z) {
                            if ((before_z < FieldNear && after_z < FieldNear) ||
                                (before_z > FieldFar && after_z > FieldFar)) {
                                return;
                            }

                            const auto start = edge_at(section_start, before_z);
                            const auto end = edge_at(section_end, after_z);
                            Quad{
                                Vec2{ start.left, start.y },
                                Vec2{ start.right, start.y },
                                Vec2{ end.right, end.y },
                                Vec2{ end.left, end.y }
                            }.draw(start.color, start.color, end.color, end.color);
                            Line{ (start.left + start.right) * 0.5, start.y, (end.left + end.right) * 0.5, end.y }.draw(
                                5.0,
                                center_color
                            );
                        }
                    );
                }
            }
        }

        void DrawSliderHoldHeads(
            const sheets::Chart& chart,
            const playfield::ChartProjection::Frame& projection,
            const double note_speed,
            const core::judgement::JudgeEngine* judge_engine
        ) {
            constexpr double SourceHeight = FieldSurfaceSize.y;
            constexpr double SourceStageLeft = 32.0;
            constexpr double SourceLaneWidth = (FieldSurfaceSize.x - SourceStageLeft * 2.0) / 16.0;
            const ColorF light = ColorF{ U"#55C8FF" }.removeSRGBCurve();
            const ColorF dark = ColorF{ U"#43A4F2" }.removeSRGBCurve();

            for (std::size_t hold_index = 0; hold_index < chart.slider_holds.size(); ++hold_index) {
                const auto& hold = chart.slider_holds[hold_index];
                for (const auto& point : hold.points) {
                    if (point.kind != sheets::SliderHoldPointKind::Start &&
                        point.kind != sheets::SliderHoldPointKind::End) {
                        continue;
                    }
                    if (judge_engine && point.kind == sheets::SliderHoldPointKind::Start &&
                        judge_engine->sliderHoldPointState(hold_index, 0) != core::judgement::NoteState::Pending) {
                        continue;
                    }
                    const double z = WorldZ(projection, point.timeline, point.sample, note_speed);
                    if (!std::isfinite(z)) {
                        continue;
                    }
                    const double y = (FieldFar - z) * SourceHeight / FieldLength;
                    if (y < -NoteVisibilityMargin || y > SourceHeight + NoteVisibilityMargin) {
                        continue;
                    }

                    const Vec2 left_center{ SourceStageLeft + point.lane.start * SourceLaneWidth, y };
                    const double width = point.lane.width * SourceLaneWidth;
                    DrawRoundedNotePlate(left_center, width, NotePlateHeight, light, dark);
                }
            }
        }

        void DrawSliderNotes(
            const sheets::Chart& chart,
            const playfield::ChartProjection::Frame& projection,
            const double note_speed,
            const core::judgement::JudgeEngine* judge_engine
        ) {
            constexpr double SourceStageLeft = 32.0;
            constexpr double SourceLaneWidth = (FieldSurfaceSize.x - SourceStageLeft * 2.0) / 16.0;

            for (std::size_t index = 0; index < chart.slider_notes.size(); ++index) {
                if (judge_engine && judge_engine->noteState(index) != core::judgement::NoteState::Pending) {
                    continue;
                }
                const auto& note = chart.slider_notes[index];
                const double z = WorldZ(projection, note.timeline, note.sample, note_speed);
                if (!std::isfinite(z)) {
                    continue;
                }
                const double y = (FieldFar - z) * FieldSurfaceSize.y / FieldLength;
                if (y < -NoteVisibilityMargin || y > FieldSurfaceSize.y + NoteVisibilityMargin) {
                    continue;
                }

                ColorF light{ U"#FFFFFF" };
                ColorF dark{ U"#BEE4EF" };
                switch (note.kind) {
                    case sheets::SliderNoteKind::Tap:
                        break;
                    case sheets::SliderNoteKind::XTap:
                        light = ColorF{ U"#FEEFB4" };
                        dark = ColorF{ U"#FEDF64" };
                        break;
                    case sheets::SliderNoteKind::Flick:
                        light = ColorF{ U"#D5A1F0" };
                        dark = ColorF{ U"#B429FA" };
                        break;
                }

                const Vec2 left_center{ SourceStageLeft + note.lane.start * SourceLaneWidth, y };
                DrawRoundedNotePlate(
                    left_center,
                    note.lane.width * SourceLaneWidth,
                    NotePlateHeight,
                    light.removeSRGBCurve(),
                    dark.removeSRGBCurve(),
                    note.kind == sheets::SliderNoteKind::Tap || note.kind == sheets::SliderNoteKind::XTap
                );
            }
        }

        void DrawSliderTouchEffects(const uint32 slider_pressed) {
            constexpr double SourceStageLeft = 32.0;
            constexpr double SourceLaneWidth = (FieldSurfaceSize.x - SourceStageLeft * 2.0) / 16.0;
            constexpr double TouchHeight = 250.0;
            for (uint32 lane = 0; lane < 16; ++lane) {
                if ((slider_pressed & (uint32{ 3 } << (lane * 2))) == 0) {
                    continue;
                }
                const double x = SourceStageLeft + SourceLaneWidth * lane;
                RectF{ Arg::bottomLeft(x, FieldSurfaceSize.y), SourceLaneWidth, TouchHeight }.draw(
                    Arg::top = ColorF{ 1.0, 0.0 },
                    Arg::bottom = ColorF{ 1.0, 0.7 }
                );
            }
        }

        void DrawFieldSurface(
            const uint32 combo,
            const sheets::Chart& chart,
            const playfield::ChartProjection::Frame& projection,
            const int64 current_sample,
            const double note_speed,
            const core::judgement::JudgeEngine* judge_engine,
            const uint32 slider_pressed,
            const std::span<const double> measure_depths,
            Array<int64>& section_samples
        ) {
            constexpr double SourceWidth = FieldSurfaceSize.x;
            constexpr double SourceHeight = FieldSurfaceSize.y;
            constexpr double SourceStageLeft = 32.0;
            constexpr double SourceLaneWidth = (SourceWidth - SourceStageLeft * 2.0) / 16.0;
            const ColorF judge_color = ColorF{ U"#79C8FF" }.removeSRGBCurve();

            // The 2D surface is sampled by the linear-color 3D pass. Convert sRGB artwork colors here once.
            // Same artwork and placement as the current Game renderer, excluding its far-end fade.
            RectF{ 0, 0, SourceWidth, SourceHeight }.draw(ColorF{ U"#212830" }.removeSRGBCurve());
            for (int32 lane = 4; lane < 16; lane += 4) {
                const double x = SourceStageLeft + lane * SourceLaneWidth;
                Line{ x, 0, x, SourceHeight }.draw(2.5, ColorF{ U"#FDFDFD" }.removeSRGBCurve());
            }
            for (const double z : measure_depths) {
                if (z < FieldNear) {
                    continue;
                }
                const double y = (FieldFar - z) * SourceHeight / FieldLength;
                Line{ 0.0, y, SourceWidth, y }.draw(2.0, ColorF{ U"#A8BCC8" }.removeSRGBCurve().withA(0.5));
            }
            DrawSliderTouchEffects(slider_pressed);
            DrawSliderHoldBodies(
                chart,
                projection,
                current_sample,
                note_speed,
                judge_engine,
                slider_pressed,
                section_samples
            );
            DrawSliderHoldHeads(chart, projection, note_speed, judge_engine);
            DrawSliderNotes(chart, projection, note_speed, judge_engine);
            if (combo >= 5) {
                FontAsset{ assets::font::ComboNumber }(combo)
                    .drawAt(SourceWidth * 0.5, SourceHeight - 750.0, Palette::White);
                FontAsset{ assets::font::Display }(U"COMBO")
                    .drawAt(SourceWidth * 0.5, SourceHeight - 600.0, Palette::White);
            }
            static const Polygon LeftJudgeSparkle =
                primitives::Sparkle(Vec2{ SourceStageLeft - 2.0, SourceJudgeY }, 200.0, 300.0, 0.0, 16.0).asPolygon();
            static const Polygon RightJudgeSparkle =
                primitives::Sparkle(Vec2{ SourceWidth - SourceStageLeft + 2.0, SourceJudgeY }, 200.0, 300.0, 0.0, 16.0)
                    .asPolygon();
            LeftJudgeSparkle.drawFrame(1.5, judge_color);
            RightJudgeSparkle.drawFrame(1.5, judge_color);
            RectF{ 0, SourceJudgeY - 2.5, SourceWidth, 5.0 }
                .drawShadow(Vec2{ 0, 10 }, 30.0, 0.0, judge_color.withA(0.15))
                .draw(judge_color);
            RectF{ 0, 0, SourceStageLeft, SourceHeight }.draw(ColorF{ U"#B4E6FF" }.removeSRGBCurve());
            RectF{ SourceWidth - SourceStageLeft, 0, SourceStageLeft, SourceHeight }.draw(
                ColorF{ U"#B4E6FF" }.removeSRGBCurve()
            );
        }

        void DrawSideSurfaces() {
            const ColorF lower_color = SideLowerColor.removeSRGBCurve();
            const ColorF upper_color = SideUpperColor.removeSRGBCurve();
            const ColorF edge_color = ColorF{ U"#B4E6FF" }.removeSRGBCurve();
            for (const double side : { -1.0, 1.0 }) {
                // The lower side continues toward the camera beyond the judgement bar.
                OrientedBox{
                    Vec3{ side * SideLowerX, SideLowerY, LowerCenter }, LowerWidth, 0.12, LowerLength, SideSlope(side),
                }
                    .draw(lower_color);
                Box{ Vec3{ side * SideUpperX, SideUpperY, FieldCenter }, 0.12, SideUpperHeight, FieldLength }.draw(
                    upper_color
                );
                Box{ Vec3{ side * SideUpperX, SideLowerRise + 0.02, LowerCenter }, 0.06, 0.06, LowerLength }.draw(
                    edge_color
                );
            }
        }

        void DrawSideTouchEffects(const uint8 side_pressed) {
            if (side_pressed == 0) {
                return;
            }

            constexpr int32 SegmentCount = 24;
            constexpr double TouchFar = FieldNear + FieldLength * 250.0 / FieldSurfaceSize.y;
            const ColorF touch_color = ColorF{ U"#D45CD7" }.removeSRGBCurve();
            const ScopedRenderStates3D translucent{ BlendState::NonPremultiplied, DepthStencilState::DepthTest };
            for (const auto button : { sheets::SideButton::LeftUpper,
                                       sheets::SideButton::RightUpper,
                                       sheets::SideButton::LeftLower,
                                       sheets::SideButton::RightLower }) {
                if ((side_pressed & (uint8{ 1 } << static_cast<uint8>(button))) == 0) {
                    continue;
                }

                const bool left = button == sheets::SideButton::LeftUpper || button == sheets::SideButton::LeftLower;
                const double side = left ? -1.0 : 1.0;
                const bool upper = button == sheets::SideButton::LeftUpper || button == sheets::SideButton::RightUpper;
                const double near = upper ? FieldNear : LowerNear;
                const double segment_depth = (TouchFar - near) / SegmentCount;
                for (int32 index = 0; index < SegmentCount; ++index) {
                    const double z = near + segment_depth * (index + 0.5);
                    const ColorF color = touch_color.withA(0.65 * (1.0 - (index + 0.5) / SegmentCount));
                    if (upper) {
                        const Quaternion facing_inward = Quaternion::RotateZ(side > 0.0 ? 90_deg : -90_deg);
                        Plane{ Vec3{ side * (SideUpperX - 0.09), SideUpperY, z }, SideUpperHeight, segment_depth }.draw(
                            facing_inward,
                            color
                        );
                    } else {
                        const Quaternion facing_inward = Quaternion::RotateZ(side * SideLowerAngle);
                        Plane{ SideLowerPosition(side, z, 0.08), LowerWidth, segment_depth }.draw(facing_inward, color);
                    }
                }
            }
        }

        void DrawSideCrossSection(const double z, const double depth, const ColorF& color) {
            for (const double side : { -1.0, 1.0 }) {
                if (z >= LowerNear) {
                    OrientedBox{ SideLowerPosition(side, z, 0.07), LowerWidth, 0.08, depth, SideSlope(side) }.draw(
                        color
                    );
                }
                if (z >= FieldNear) {
                    Box{ Vec3{ side * SideUpperX, SideUpperY, z }, 0.16, SideUpperHeight, depth }.draw(color);
                }
            }
        }

        void DrawField(
            const MSRenderTexture& surface,
            const bool show_depth_guides,
            const uint8 side_pressed,
            const std::span<const double> measure_depths
        ) {
            const ColorF floor_color = ColorF{ U"#212830" }.removeSRGBCurve();
            const ColorF judge_color = ColorF{ U"#79C8FF" }.removeSRGBCurve();

            Box{ Vec3{ 0.0, -0.06, FieldCenter }, FieldWidth, 0.12, FieldLength }.draw(floor_color);
            Plane{ Vec3{ 0.0, 0.02, FieldCenter }, FieldWidth, FieldLength }.draw(surface);
            DrawSideSurfaces();
            // Use exactly the same world depths as the main field's textured lines.
            {
                // Background markings must not occlude hold bodies or notes drawn later.
                const ScopedRenderStates3D depth{ DepthStencilState::DepthTest };
                for (const double z : measure_depths) {
                    DrawSideCrossSection(z, 0.06, ColorF{ U"#A8BCC8" }.removeSRGBCurve());
                }
            }
            DrawSideTouchEffects(side_pressed);

            // Side judgement strips meet the main field's textured judgement line at world depth zero.
            DrawSideCrossSection(0.0, 0.10, judge_color);

            if (!show_depth_guides) {
                return;
            }
            // Optional depth markers help compare the perspective of the main and side surfaces.
            for (double z = 20.0; z <= FieldFar; z += 20.0) {
                Box{ Vec3{ 0.0, 0.08, z }, FieldWidth, 0.08, 0.15 }.draw(judge_color);
                DrawSideCrossSection(z, 0.15, judge_color);
            }
        }

        [[nodiscard]]
        Optional<double> NoteDepth(
            const playfield::ChartProjection::Frame& projection,
            const sheets::TimelineIndex timeline,
            const int64 note_sample,
            const double note_speed,
            const double near_limit = FieldNear
        ) {
            const double z = WorldZ(projection, timeline, note_sample, note_speed);
            if (!std::isfinite(z) || z < near_limit - 2.0 || z > FieldFar + 2.0) {
                return none;
            }
            return z;
        }

        void DrawSliderHoldDiamonds(
            const sheets::Chart& chart,
            const playfield::ChartProjection::Frame& projection,
            const double note_speed
        ) {
            constexpr double Edge = 0.4;
            constexpr double HalfDiagonal = Edge * 0.7071067811865476;
            const ColorF color = ColorF{ U"#D9D9D9" }.removeSRGBCurve();

            for (const auto& hold : chart.slider_holds) {
                for (const auto& point : hold.points) {
                    if (point.kind != sheets::SliderHoldPointKind::Visible) {
                        continue;
                    }
                    const auto z = NoteDepth(projection, point.timeline, point.sample, note_speed);
                    if (!z) {
                        continue;
                    }

                    const double x =
                        -FieldWidth * 0.5 + StageLeft + (point.lane.start + point.lane.width * 0.5) * LaneWidth;
                    OrientedBox{ Vec3{ x, 0.04 + HalfDiagonal, *z }, Edge, Edge, 0.12, Quaternion::RotateZ(45_deg) }
                        .draw(color);
                }
            }
        }

        void DrawSideNoteSurface(
            MSRenderTexture& surface,
            const ColorF& background,
            const ColorF& light,
            const ColorF& dark,
            const ColorF& outline
        ) {
            // Keep the texture opaque so the existing 3D depth and blend states remain unchanged.
            const ScopedRenderTarget2D target{ surface.clear(background.removeSRGBCurve()) };
            const Vec2 left_center{ 0.0, SideNoteSurfaceSize.y * 0.5 };
            const Transformer2D transform{ Mat3x2::Rotate(-90_deg, left_center) };
            const RoundRect plate{ left_center.movedBy(-SideNoteSurfaceSize.y * 0.5, 0),
                                   SideNoteSurfaceSize.y,
                                   SideNoteSurfaceSize.x,
                                   12.0 };
            plate.draw(outline.removeSRGBCurve());
            plate.stretched(-4).draw(Arg::top = light.removeSRGBCurve(), Arg::bottom = dark.removeSRGBCurve());
            plate.stretched(-7).drawFrame(1.5, ColorF{ 1.0 });
            Graphics2D::Flush();
            surface.resolve();
        }

        void DrawSideNotePlane(
            const sheets::SideButton button,
            const double z,
            const MSRenderTexture& upper_surface,
            const MSRenderTexture& lower_surface
        ) {
            const bool left = button == sheets::SideButton::LeftUpper || button == sheets::SideButton::LeftLower;
            const double side = left ? -1.0 : 1.0;
            const bool upper = button == sheets::SideButton::LeftUpper || button == sheets::SideButton::RightUpper;
            if (upper) {
                const Quaternion facing_inward = Quaternion::RotateZ(side > 0.0 ? 90_deg : -90_deg);
                Plane{ Vec3{ side * (SideUpperX - 0.12), SideUpperY, z }, SideUpperHeight, SideNoteDepthSize }.draw(
                    facing_inward,
                    upper_surface
                );
            } else {
                const Quaternion facing_inward = Quaternion::RotateZ(side * SideLowerAngle);
                Plane{ SideLowerPosition(side, z, 0.12), LowerWidth, SideNoteDepthSize }.draw(
                    facing_inward,
                    lower_surface
                );
            }
        }

        void DrawSideHoldBodies(
            const sheets::Chart& chart,
            const playfield::ChartProjection::Frame& projection,
            const int64 current_sample,
            const double note_speed,
            const core::judgement::JudgeEngine* judge_engine,
            const uint8 side_pressed,
            Array<int64>& section_samples
        ) {
            const ScopedRenderStates3D translucent{ BlendState::NonPremultiplied, DepthStencilState::DepthTest };
            for (std::size_t hold_index = 0; hold_index < chart.side_holds.size(); ++hold_index) {
                const auto& hold = chart.side_holds[hold_index];
                if (judge_engine && !hold.judge_samples.isEmpty() &&
                    judge_engine->sideHoldPointState(hold_index, hold.judge_samples.size() - 1) !=
                        core::judgement::NoteState::Pending &&
                    current_sample >= hold.judge_samples.back()) {
                    continue;
                }
                const bool left =
                    hold.button == sheets::SideButton::LeftUpper || hold.button == sheets::SideButton::LeftLower;
                const double side = left ? -1.0 : 1.0;
                const bool upper =
                    hold.button == sheets::SideButton::LeftUpper || hold.button == sheets::SideButton::RightUpper;
                const ColorF color =
                    (upper ? SideUpperNoteDarkColor : SideLowerNoteDarkColor).removeSRGBCurve().withA(0.6);
                const bool active = SideHoldActive(hold, hold_index, current_sample, side_pressed, judge_engine);
                for (std::size_t index = 1; index < hold.points.size(); ++index) {
                    const auto& before = hold.points[index - 1];
                    const auto& after = hold.points[index];
                    ForEachHoldSection(
                        projection,
                        note_speed,
                        before,
                        after,
                        section_samples,
                        [&](const int64, const double before_z, const int64, const double after_z) {
                            const double near_limit = active ? 0.0 : (upper ? FieldNear : LowerNear);
                            const double near_z = Clamp(Min(before_z, after_z), near_limit, FieldFar);
                            const double far_z = Clamp(Max(before_z, after_z), near_limit, FieldFar);
                            if (far_z - near_z < 0.01) {
                                return;
                            }
                            const double center_z = (near_z + far_z) * 0.5;
                            if (upper) {
                                const Quaternion facing_inward = Quaternion::RotateZ(side > 0.0 ? 90_deg : -90_deg);
                                Plane{ Vec3{ side * (SideUpperX - 0.09), SideUpperY, center_z },
                                       SideUpperHeight,
                                       far_z - near_z }
                                    .draw(facing_inward, color);
                            } else {
                                const Quaternion facing_inward = Quaternion::RotateZ(side * SideLowerAngle);
                                Plane{ SideLowerPosition(side, center_z, 0.09), LowerWidth, far_z - near_z }.draw(
                                    facing_inward,
                                    color
                                );
                            }
                        }
                    );
                }
            }
        }

        void DrawSideNotes(
            const sheets::Chart& chart,
            const playfield::ChartProjection::Frame& projection,
            const double note_speed,
            const MSRenderTexture& upper_surface,
            const MSRenderTexture& lower_surface,
            const MSRenderTexture& upper_xtap_surface,
            const MSRenderTexture& lower_xtap_surface,
            const core::judgement::JudgeEngine* judge_engine
        ) {
            // Draw hold markers first so co-located XTap notes remain visible.
            for (std::size_t hold_index = 0; hold_index < chart.side_holds.size(); ++hold_index) {
                const auto& hold = chart.side_holds[hold_index];
                const bool lower =
                    hold.button == sheets::SideButton::LeftLower || hold.button == sheets::SideButton::RightLower;
                for (const auto& point : hold.points) {
                    if (judge_engine && point.kind == sheets::SideHoldPointKind::Start &&
                        judge_engine->sideHoldPointState(hold_index, 0) != core::judgement::NoteState::Pending) {
                        continue;
                    }
                    const auto z =
                        NoteDepth(projection, point.timeline, point.sample, note_speed, lower ? LowerNear : FieldNear);
                    if (z) {
                        DrawSideNotePlane(hold.button, *z, upper_surface, lower_surface);
                    }
                }
            }
            for (std::size_t index = 0; index < chart.side_notes.size(); ++index) {
                if (judge_engine && judge_engine->sideNoteState(index) != core::judgement::NoteState::Pending) {
                    continue;
                }
                const auto& note = chart.side_notes[index];
                const bool lower =
                    note.button == sheets::SideButton::LeftLower || note.button == sheets::SideButton::RightLower;
                const auto z =
                    NoteDepth(projection, note.timeline, note.sample, note_speed, lower ? LowerNear : FieldNear);
                if (!z) {
                    continue;
                }
                const bool xtap = note.kind == sheets::SideNoteKind::XTap;
                DrawSideNotePlane(
                    note.button,
                    *z,
                    xtap ? upper_xtap_surface : upper_surface,
                    xtap ? lower_xtap_surface : lower_surface
                );
            }
        }
    }

    PlayfieldRenderer3D::PlayfieldRenderer3D()
        : m_field_surface{ FieldSurfaceSize, TextureFormat::R8G8B8A8_Unorm_SRGB },
          m_render_texture{ DesignSize, TextureFormat::R8G8B8A8_Unorm_SRGB, HasDepth::Yes },
          m_side_upper_note_surface{ SideNoteSurfaceSize, TextureFormat::R8G8B8A8_Unorm_SRGB },
          m_side_lower_note_surface{ SideNoteSurfaceSize, TextureFormat::R8G8B8A8_Unorm_SRGB },
          m_side_upper_xtap_surface{ SideNoteSurfaceSize, TextureFormat::R8G8B8A8_Unorm_SRGB },
          m_side_lower_xtap_surface{ SideNoteSurfaceSize, TextureFormat::R8G8B8A8_Unorm_SRGB }, m_fog_shader{
              HLSL{ Resource(U"ui/shaders/hlsl/playfield_fog.hlsl"), U"PS" } |
              GLSL{ Resource(U"ui/shaders/glsl/playfield_fog.frag"),
                    { { U"PSPerFrame", 0 }, { U"PSPerView", 1 }, { U"PSPerMaterial", 3 }, { U"PSFog", 4 } } }
          } {
        if (!m_fog_shader) {
            throw Error{ U"Failed to load the 3D playfield depth-fade shader." };
        }

        const ColorF fog_color = BackgroundColor.removeSRGBCurve();
        m_fog_parameters->color_and_start = Float4{ static_cast<float>(fog_color.r),
                                                    static_cast<float>(fog_color.g),
                                                    static_cast<float>(fog_color.b),
                                                    static_cast<float>(FadeStart) };
        m_fog_parameters->end_and_strength =
            Float4{ static_cast<float>(FieldFar), static_cast<float>(FadeStrength), 0.0f, 0.0f };

        DrawSideNoteSurface(
            m_side_upper_note_surface,
            SideUpperColor,
            SideUpperNoteLightColor,
            SideUpperNoteDarkColor,
            SideUpperNoteOutlineColor
        );
        DrawSideNoteSurface(
            m_side_lower_note_surface,
            SideLowerColor,
            SideLowerNoteLightColor,
            SideLowerNoteDarkColor,
            SideLowerNoteOutlineColor
        );
        DrawSideNoteSurface(
            m_side_upper_xtap_surface,
            SideUpperColor,
            ColorF{ U"#FFECA0" },
            ColorF{ U"#F4B900" },
            ColorF{ U"#8A4B00" }
        );
        DrawSideNoteSurface(
            m_side_lower_xtap_surface,
            SideLowerColor,
            ColorF{ U"#FFECA0" },
            ColorF{ U"#F4B900" },
            ColorF{ U"#8A4B00" }
        );
    }

    void PlayfieldRenderer3D::addJudgements(
        const sheets::Chart& chart,
        const std::span<const core::judgement::JudgementEvent> events
    ) {
        for (const auto& event : events) {
            const auto hit = TapHitForJudgement(chart, event);
            if (!hit) {
                continue;
            }
            TapHitSurface surface;
            if (hit->side_button) {
                const auto button = *hit->side_button;
                const bool left = button == sheets::SideButton::LeftUpper || button == sheets::SideButton::LeftLower;
                const bool upper = button == sheets::SideButton::LeftUpper || button == sheets::SideButton::RightUpper;
                const double side = left ? -1.0 : 1.0;
                if (upper) {
                    surface.center = Vec3{ side * (SideUpperX - 0.16), SideUpperY, 0.0 };
                    surface.rotation = Quaternion::RotateZ(side > 0.0 ? 90_deg : -90_deg);
                    surface.width = SideUpperHeight;
                } else {
                    surface.center = SideLowerPosition(side, 0.0, 0.16);
                    surface.rotation = Quaternion::RotateZ(side * SideLowerAngle);
                    surface.width = LowerWidth;
                }
            } else {
                const double x = -FieldWidth * 0.5 + StageLeft + (hit->lane.start + hit->lane.width * 0.5) * LaneWidth;
                surface.center = Vec3{ x, 0.10, 0.0 };
                surface.width = hit->lane.width * LaneWidth;
            }
            m_hit_effects.add(*hit, surface);
        }
    }

    void PlayfieldRenderer3D::draw(
        const BasicCamera3D& camera,
        const sheets::Chart& chart,
        const playfield::ChartProjection& projection,
        const int64 current_sample,
        const double note_speed,
        const uint32 combo,
        const bool show_depth_guides,
        const core::judgement::JudgeEngine* judge_engine,
        const uint32 slider_pressed,
        const uint8 side_pressed
    ) const {
        const auto frame = projection.frameAt(current_sample, m_current_positions);
        UpdateMeasureLineDepths(chart, frame, note_speed, m_measure_depths);
        {
            const ScopedRenderTarget2D target{ m_field_surface.clear(ColorF{ 0.0, 0.0 }) };
            DrawFieldSurface(
                combo,
                chart,
                frame,
                current_sample,
                note_speed,
                judge_engine,
                slider_pressed,
                m_measure_depths,
                m_hold_section_samples
            );
            Graphics2D::Flush();
            m_field_surface.resolve();
        }

        Graphics3D::SetCameraTransform(camera);
        Graphics3D::SetGlobalAmbientColor(ColorF{ 1.0 });
        Graphics3D::SetSunColor(ColorF{ 0.0 });
        Graphics3D::SetPSConstantBuffer(4, m_fog_parameters);
        {
            const ScopedRenderTarget3D target{ m_render_texture.clear(BackgroundColor.removeSRGBCurve()) };
            const ScopedCustomShader3D fog_shader{ m_fog_shader };
            DrawField(m_field_surface, show_depth_guides, side_pressed, m_measure_depths);
            DrawSliderHoldDiamonds(chart, frame, note_speed);
            DrawSideHoldBodies(
                chart,
                frame,
                current_sample,
                note_speed,
                judge_engine,
                side_pressed,
                m_hold_section_samples
            );
            DrawSideNotes(
                chart,
                frame,
                note_speed,
                m_side_upper_note_surface,
                m_side_lower_note_surface,
                m_side_upper_xtap_surface,
                m_side_lower_xtap_surface,
                judge_engine
            );
            m_hit_effects.draw(camera);
        }
        Graphics3D::Flush();
        m_render_texture.resolve();
        Shader::LinearToScreen(m_render_texture);
    }
}
