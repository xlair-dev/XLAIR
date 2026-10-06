#include "PlayfieldRenderer3D.hpp"

#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/primitives/Sparkle.hpp"

#include <cmath>

namespace xlair::ui::game {
    namespace {
        constexpr Size FieldSurfaceSize{ 1000, 4000 };
        constexpr Size SideNoteSurfaceSize{ 512, 64 };
        constexpr double NotePlateHeight = 32.0;
        constexpr double NoteVisibilityMargin = 64.0;
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
        constexpr double LowerWidth = 3.5355339059;
        constexpr double SideLowerX = 9.25;
        constexpr double SideLowerY = 1.25;
        constexpr double SideUpperX = 10.5;
        constexpr double SideUpperY = 4.0;
        // A side note covers the same fraction of the field's depth as a plate on the 2D surface.
        constexpr double NoteDepthSize = FieldLength * NotePlateHeight / FieldSurfaceSize.y;
        // Begin the fade gradually beyond the combo area, with the same world-z range for all 3D geometry.
        constexpr double FadeStart = FieldNear + FieldLength * 0.3;
        constexpr double FadeStrength = 0.8;
        const ColorF BackgroundColor{ U"#F7F8FC" };
        const ColorF SideUpperColor{ U"#D5F0FB" };
        const ColorF SideLowerColor{ U"#FFFFFF" };
        // Corresponds to Game's 360 px/s across its 880 px reference viewport.
        constexpr double WorldUnitsPerSecond = FieldLength * 100.0 / 880.0;

        [[nodiscard]]
        Quaternion SideSlope(const double side) {
            return Quaternion::RotateZ(side < 0.0 ? 135_deg : 45_deg);
        }

        [[nodiscard]]
        double WorldZ(
            const playfield::ChartProjection& projection,
            const sheets::TimelineIndex timeline,
            const int64 current_sample,
            const int64 note_sample,
            const double note_speed
        ) {
            // Play options scale the visual distance without changing the chart's sample clock.
            return static_cast<double>(projection.noteDistance(timeline, current_sample, note_sample)) *
                   WorldUnitsPerSecond * note_speed;
        }

        template <class Point, class DrawSection>
        void ForEachHoldSection(
            const playfield::ChartProjection& projection,
            const int64 current_sample,
            const double note_speed,
            const Point& before,
            const Point& after,
            DrawSection&& draw_section
        ) {
            if (before.sample >= after.sample) {
                return;
            }

            auto samples =
                projection.noteDistanceBreakpoints(before.timeline, current_sample, before.sample, after.sample);
            samples.push_back(after.sample);

            int64 previous_sample = before.sample;
            double previous_z = WorldZ(projection, before.timeline, current_sample, previous_sample, note_speed);
            for (const int64 sample : samples) {
                // The preceding anchor owns the interval's scroll timeline. The final
                // endpoint keeps its own timeline, matching the anchor rendering.
                const auto timeline = sample == after.sample ? after.timeline : before.timeline;
                const double z = WorldZ(projection, timeline, current_sample, sample, note_speed);
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

        void DrawSliderHoldBodies(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample,
            const double note_speed
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

            for (const auto& hold : chart.slider_holds) {
                if (hold.points.size() < 2) {
                    continue;
                }
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
                        current_sample,
                        note_speed,
                        before,
                        after,
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
            const playfield::ChartProjection& projection,
            const int64 current_sample,
            const double note_speed
        ) {
            constexpr double SourceHeight = FieldSurfaceSize.y;
            constexpr double SourceStageLeft = 32.0;
            constexpr double SourceLaneWidth = (FieldSurfaceSize.x - SourceStageLeft * 2.0) / 16.0;
            const ColorF light = ColorF{ U"#55C8FF" }.removeSRGBCurve();
            const ColorF dark = ColorF{ U"#43A4F2" }.removeSRGBCurve();

            for (const auto& hold : chart.slider_holds) {
                for (const auto& point : hold.points) {
                    if (point.kind != sheets::SliderHoldPointKind::Start &&
                        point.kind != sheets::SliderHoldPointKind::End) {
                        continue;
                    }
                    const double z = WorldZ(projection, point.timeline, current_sample, point.sample, note_speed);
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
            const playfield::ChartProjection& projection,
            const int64 current_sample,
            const double note_speed
        ) {
            constexpr double SourceStageLeft = 32.0;
            constexpr double SourceLaneWidth = (FieldSurfaceSize.x - SourceStageLeft * 2.0) / 16.0;

            for (const auto& note : chart.slider_notes) {
                const double z = WorldZ(projection, note.timeline, current_sample, note.sample, note_speed);
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

        void DrawFieldSurface(
            const uint32 combo,
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample,
            const double note_speed
        ) {
            constexpr double SourceWidth = FieldSurfaceSize.x;
            constexpr double SourceHeight = FieldSurfaceSize.y;
            constexpr double SourceStageLeft = 32.0;
            constexpr double SourceLaneWidth = (SourceWidth - SourceStageLeft * 2.0) / 16.0;
            constexpr double SourceJudgeY = SourceHeight - 100.0;
            const ColorF judge_color = ColorF{ U"#79C8FF" }.removeSRGBCurve();

            // The 2D surface is sampled by the linear-color 3D pass. Convert sRGB artwork colors here once.
            // Same artwork and placement as the current Game renderer, excluding its far-end fade.
            RectF{ 0, 0, SourceWidth, SourceHeight }.draw(ColorF{ U"#212830" }.removeSRGBCurve());
            for (int32 lane = 4; lane < 16; lane += 4) {
                const double x = SourceStageLeft + lane * SourceLaneWidth;
                Line{ x, 0, x, SourceHeight }.draw(2.5, ColorF{ U"#FDFDFD" }.removeSRGBCurve());
            }
            DrawSliderHoldBodies(chart, projection, current_sample, note_speed);
            DrawSliderHoldHeads(chart, projection, current_sample, note_speed);
            DrawSliderNotes(chart, projection, current_sample, note_speed);
            FontAsset{ assets::font::ComboNumber }(combo)
                .drawAt(SourceWidth * 0.5, SourceHeight - 750.0, Palette::White);
            FontAsset{ assets::font::Display }(U"COMBO")
                .drawAt(SourceWidth * 0.5, SourceHeight - 600.0, Palette::White);
            primitives::Sparkle(Vec2{ SourceStageLeft - 2.0, SourceJudgeY }, 200.0, 300.0, 0.0, 16.0)
                .asPolygon()
                .drawFrame(1.5, judge_color);
            primitives::Sparkle(Vec2{ SourceWidth - SourceStageLeft + 2.0, SourceJudgeY }, 200.0, 300.0, 0.0, 16.0)
                .asPolygon()
                .drawFrame(1.5, judge_color);
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
                Box{ Vec3{ side * SideUpperX, SideUpperY, FieldCenter }, 0.12, 3.0, FieldLength }.draw(upper_color);
                Box{ Vec3{ side * SideUpperX, 2.52, LowerCenter }, 0.06, 0.06, LowerLength }.draw(edge_color);
            }
        }

        void DrawSideCrossSection(const double z, const double depth, const ColorF& color) {
            for (const double side : { -1.0, 1.0 }) {
                OrientedBox{ Vec3{ side * SideLowerX, 1.32, z }, LowerWidth, 0.08, depth, SideSlope(side) }.draw(color);
                Box{ Vec3{ side * SideUpperX, SideUpperY, z }, 0.16, 3.0, depth }.draw(color);
            }
        }

        void DrawField(const MSRenderTexture& surface, const bool show_depth_guides) {
            const ColorF floor_color = ColorF{ U"#212830" }.removeSRGBCurve();
            const ColorF judge_color = ColorF{ U"#79C8FF" }.removeSRGBCurve();

            Box{ Vec3{ 0.0, -0.06, FieldCenter }, FieldWidth, 0.12, FieldLength }.draw(floor_color);
            Plane{ Vec3{ 0.0, 0.02, FieldCenter }, FieldWidth, FieldLength }.draw(surface);
            DrawSideSurfaces();

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
            const playfield::ChartProjection& projection,
            const sheets::TimelineIndex timeline,
            const int64 current_sample,
            const int64 note_sample,
            const double note_speed,
            const double near_limit = FieldNear
        ) {
            const double z = WorldZ(projection, timeline, current_sample, note_sample, note_speed);
            if (!std::isfinite(z) || z < near_limit - 2.0 || z > FieldFar + 2.0) {
                return none;
            }
            return z;
        }

        void DrawSliderHoldDiamonds(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample,
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
                    const auto z = NoteDepth(projection, point.timeline, current_sample, point.sample, note_speed);
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
            const ColorF& dark
        ) {
            // Keep the texture opaque so the existing 3D depth and blend states remain unchanged.
            const ScopedRenderTarget2D target{ surface.clear(background.removeSRGBCurve()) };
            const Vec2 left_center{ 0.0, SideNoteSurfaceSize.y * 0.5 };
            const Transformer2D transform{ Mat3x2::Rotate(-90_deg, left_center) };
            RoundRect{ left_center.movedBy(-SideNoteSurfaceSize.y * 0.5, 0),
                       SideNoteSurfaceSize.y,
                       SideNoteSurfaceSize.x,
                       12.0 }
                .draw(Arg::top = light.removeSRGBCurve(), Arg::bottom = dark.removeSRGBCurve());
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
                Plane{ Vec3{ side * (SideUpperX - 0.12), SideUpperY, z }, 3.0, NoteDepthSize }.draw(
                    facing_inward,
                    upper_surface
                );
            } else {
                const Quaternion facing_inward = Quaternion::RotateZ(side > 0.0 ? 45_deg : -45_deg);
                Plane{ Vec3{ side * (SideLowerX - 0.085), SideLowerY + 0.085, z }, LowerWidth, NoteDepthSize }.draw(
                    facing_inward,
                    lower_surface
                );
            }
        }

        void DrawSideHoldBodies(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample,
            const double note_speed
        ) {
            const ColorF color = ColorF{ U"#D45CD7" }.removeSRGBCurve().withA(0.42);
            const ScopedRenderStates3D translucent{ BlendState::NonPremultiplied, DepthStencilState::DepthTest };
            for (const auto& hold : chart.side_holds) {
                const bool left =
                    hold.button == sheets::SideButton::LeftUpper || hold.button == sheets::SideButton::LeftLower;
                const double side = left ? -1.0 : 1.0;
                const bool upper =
                    hold.button == sheets::SideButton::LeftUpper || hold.button == sheets::SideButton::RightUpper;
                for (std::size_t index = 1; index < hold.points.size(); ++index) {
                    const auto& before = hold.points[index - 1];
                    const auto& after = hold.points[index];
                    ForEachHoldSection(
                        projection,
                        current_sample,
                        note_speed,
                        before,
                        after,
                        [&](const int64, const double before_z, const int64, const double after_z) {
                            const double near_limit = upper ? FieldNear : LowerNear;
                            const double near_z = Clamp(Min(before_z, after_z), near_limit, FieldFar);
                            const double far_z = Clamp(Max(before_z, after_z), near_limit, FieldFar);
                            if (far_z - near_z < 0.01) {
                                return;
                            }
                            const double center_z = (near_z + far_z) * 0.5;
                            if (upper) {
                                const Quaternion facing_inward = Quaternion::RotateZ(side > 0.0 ? 90_deg : -90_deg);
                                Plane{ Vec3{ side * (SideUpperX - 0.09), SideUpperY, center_z }, 3.0, far_z - near_z }
                                    .draw(facing_inward, color);
                            } else {
                                const Quaternion facing_inward = Quaternion::RotateZ(side > 0.0 ? 45_deg : -45_deg);
                                Plane{ Vec3{ side * (SideLowerX - 0.065), SideLowerY + 0.065, center_z },
                                       LowerWidth,
                                       far_z - near_z }
                                    .draw(facing_inward, color);
                            }
                        }
                    );
                }
            }
        }

        void DrawSideNotes(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample,
            const double note_speed,
            const MSRenderTexture& upper_surface,
            const MSRenderTexture& lower_surface
        ) {
            for (const auto& note : chart.side_notes) {
                const bool lower =
                    note.button == sheets::SideButton::LeftLower || note.button == sheets::SideButton::RightLower;
                const auto z = NoteDepth(
                    projection,
                    note.timeline,
                    current_sample,
                    note.sample,
                    note_speed,
                    lower ? LowerNear : FieldNear
                );
                if (!z) {
                    continue;
                }
                DrawSideNotePlane(note.button, *z, upper_surface, lower_surface);
            }
            for (const auto& hold : chart.side_holds) {
                const bool lower =
                    hold.button == sheets::SideButton::LeftLower || hold.button == sheets::SideButton::RightLower;
                for (const auto& point : hold.points) {
                    const auto z = NoteDepth(
                        projection,
                        point.timeline,
                        current_sample,
                        point.sample,
                        note_speed,
                        lower ? LowerNear : FieldNear
                    );
                    if (z) {
                        DrawSideNotePlane(hold.button, *z, upper_surface, lower_surface);
                    }
                }
            }
        }
    }

    PlayfieldRenderer3D::PlayfieldRenderer3D()
        : m_field_surface{ FieldSurfaceSize, TextureFormat::R8G8B8A8_Unorm_SRGB },
          m_render_texture{ DesignSize, TextureFormat::R8G8B8A8_Unorm_SRGB, HasDepth::Yes },
          m_side_upper_note_surface{ SideNoteSurfaceSize, TextureFormat::R8G8B8A8_Unorm_SRGB },
          m_side_lower_note_surface{ SideNoteSurfaceSize, TextureFormat::R8G8B8A8_Unorm_SRGB }, m_fog_shader{
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

        DrawSideNoteSurface(m_side_upper_note_surface, SideUpperColor, ColorF{ U"#F489E9" }, ColorF{ U"#D45CD7" });
        DrawSideNoteSurface(m_side_lower_note_surface, SideLowerColor, ColorF{ U"#F489E9" }, ColorF{ U"#D45CD7" });
    }

    void PlayfieldRenderer3D::draw(
        const BasicCamera3D& camera,
        const sheets::Chart& chart,
        const playfield::ChartProjection& projection,
        const int64 current_sample,
        const double note_speed,
        const uint32 combo,
        const bool show_depth_guides
    ) const {
        {
            const ScopedRenderTarget2D target{ m_field_surface.clear(ColorF{ 0.0, 0.0 }) };
            DrawFieldSurface(combo, chart, projection, current_sample, note_speed);
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
            DrawField(m_field_surface, show_depth_guides);
            DrawSliderHoldDiamonds(chart, projection, current_sample, note_speed);
            DrawSideHoldBodies(chart, projection, current_sample, note_speed);
            DrawSideNotes(
                chart,
                projection,
                current_sample,
                note_speed,
                m_side_upper_note_surface,
                m_side_lower_note_surface
            );
        }
        Graphics3D::Flush();
        m_render_texture.resolve();
        Shader::LinearToScreen(m_render_texture);
    }
}
