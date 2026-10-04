#include "PlayfieldDebug.hpp"

#ifndef NDEBUG
#include "ui/assets/Assets.hpp"

#include <cmath>

namespace xlair::ui::scenes {
    namespace {
        constexpr Size FieldSurfaceSize{ 1000, 4000 };
        constexpr double FieldFar = 129.2447;
        // Game's judgement line is 100 / 4000 of the field length from the near edge.
        constexpr double FieldNear = -FieldFar * 100.0 / 3900.0;
        constexpr double FieldLength = FieldFar - FieldNear;
        constexpr double FieldCenter = (FieldNear + FieldFar) * 0.5;
        constexpr double LowerNear = FieldNear - 8.0;
        constexpr double LowerLength = FieldFar - LowerNear;
        constexpr double LowerCenter = (LowerNear + FieldFar) * 0.5;
        constexpr double FieldWidth = 16.0;
        constexpr double StageLeft = FieldWidth * 32.0 / 1000.0;
        constexpr double LaneWidth = (FieldWidth - StageLeft * 2.0) / 16.0;
        constexpr double LowerWidth = 3.5355339059;
        constexpr double SideLowerX = 9.25;
        constexpr double SideLowerY = 1.25;
        constexpr double SideUpperX = 10.5;
        constexpr double SideUpperY = 4.0;
        // Corresponds to Game's 360 px/s across its 880 px reference viewport.
        constexpr double WorldUnitsPerSecond = FieldLength * 360.0 / 880.0;
        constexpr double LoopDuration = 8.0;
        constexpr int64 SampleRate = 48'000;
        // Fit the main field to Game's top (±110) and bottom (±735) screen-space corners.
        constexpr Vec3 InitialEye{ 0.0, 12.9974, -21.9358 };
        constexpr Vec3 InitialFocus{ 0.0, 0.0, 13.9422 };

        [[nodiscard]]
        Quaternion SideSlope(const double side) {
            return Quaternion::RotateZ(side < 0.0 ? 135_deg : 45_deg);
        }

        void DrawFieldSurface(const uint32 combo) {
            constexpr double SourceWidth = FieldSurfaceSize.x;
            constexpr double SourceHeight = FieldSurfaceSize.y;
            constexpr double SourceStageLeft = 32.0;
            constexpr double SourceLaneWidth = (SourceWidth - SourceStageLeft * 2.0) / 16.0;
            constexpr double SourceJudgeY = SourceHeight - 100.0;

            // The 2D surface is sampled by the linear-color 3D pass. Convert sRGB artwork colors here once.
            // Same artwork and placement as the current Game renderer, excluding its far-end fade.
            RectF{ 0, 0, SourceWidth, SourceHeight }.draw(ColorF{ U"#212830" }.removeSRGBCurve());
            for (int32 lane = 4; lane < 16; lane += 4) {
                const double x = SourceStageLeft + lane * SourceLaneWidth;
                Line{ x, 0, x, SourceHeight }.draw(2.5, ColorF{ U"#FDFDFD" }.removeSRGBCurve());
            }
            FontAsset{ assets::font::ComboNumber }(combo)
                .drawAt(SourceWidth * 0.5, SourceHeight - 750.0, Palette::White);
            FontAsset{ assets::font::Display }(U"COMBO")
                .drawAt(SourceWidth * 0.5, SourceHeight - 600.0, Palette::White);
            RectF{ 0, SourceJudgeY - 2.5, SourceWidth, 5.0 }
                .drawShadow(Vec2{ 0, 10 }, 30.0, 0.0, ColorF{ U"#79C8FF" }.removeSRGBCurve().withA(0.15))
                .draw(ColorF{ U"#79C8FF" }.removeSRGBCurve());
            RectF{ 0, 0, SourceStageLeft, SourceHeight }.draw(ColorF{ U"#B4E6FF" }.removeSRGBCurve());
            RectF{ SourceWidth - SourceStageLeft, 0, SourceStageLeft, SourceHeight }.draw(
                ColorF{ U"#B4E6FF" }.removeSRGBCurve()
            );
        }

        [[nodiscard]]
        sheets::Chart MakeDebugChart() {
            sheets::Chart chart;
            chart.sample_rate = SampleRate;
            chart.timelines.push_back({});
            chart.timelines.push_back({
                .speed_changes = {
                    { .sample = 0, .multiplier = 1.0 },
                    { .sample = 2 * SampleRate, .multiplier = 0.5 },
                    { .sample = 4 * SampleRate, .multiplier = 1.5 },
                },
            });

            for (int32 index = 0; index < 4; ++index) {
                const int64 sample = (index + 1) * 3 * SampleRate / 2;
                const sheets::TimelineIndex timeline = index % 2;
                chart.slider_notes.push_back(
                    {
                        .kind = sheets::SliderNoteKind::Tap,
                        .timeline = timeline,
                        .sample = sample,
                        .lane = { .start = static_cast<uint8>(index % 2 == 0 ? 2 : 10), .width = 4 },
                    }
                );
                for (const auto button : {
                         sheets::SideButton::LeftUpper,
                         sheets::SideButton::LeftLower,
                         sheets::SideButton::RightUpper,
                         sheets::SideButton::RightLower,
                     }) {
                    chart.side_notes.push_back({ .timeline = timeline, .sample = sample, .button = button });
                }
            }
            return chart;
        }

        void DrawSideSurfaces() {
            const ColorF lower_color = ColorF{ U"#FFFFFF" }.removeSRGBCurve();
            const ColorF upper_color = ColorF{ U"#D5F0FB" }.removeSRGBCurve();
            const ColorF edge_color = ColorF{ U"#B4E6FF" }.removeSRGBCurve();
            for (const double side : { -1.0, 1.0 }) {
                // The lower side continues toward the camera beyond the judgement bar.
                OrientedBox{ Vec3{ side * SideLowerX, SideLowerY, LowerCenter },
                             LowerWidth,
                             0.12,
                             LowerLength,
                             SideSlope(side) }
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
            const int64 note_sample
        ) {
            const double z = static_cast<double>(projection.noteDistance(timeline, current_sample, note_sample)) *
                             WorldUnitsPerSecond;
            if (!std::isfinite(z) || z < FieldNear - 2.0 || z > FieldFar + 2.0) {
                return none;
            }
            return z;
        }

        [[nodiscard]]
        ColorF NoteColor(const sheets::TimelineIndex timeline) {
            return ColorF{ timeline == 0 ? U"#D45CD7" : U"#F5A623" }.removeSRGBCurve();
        }

        void DrawSliderNotes(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample
        ) {
            for (const auto& note : chart.slider_notes) {
                const auto z = NoteDepth(projection, note.timeline, current_sample, note.sample);
                if (!z) {
                    continue;
                }
                const double x = -FieldWidth * 0.5 + StageLeft + (note.lane.start + note.lane.width * 0.5) * LaneWidth;
                Box{ Vec3{ x, 0.18, *z }, note.lane.width * LaneWidth * 0.9, 0.2, 0.6 }.draw(NoteColor(note.timeline));
            }
        }

        void DrawSideNotes(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample
        ) {
            for (const auto& note : chart.side_notes) {
                const auto z = NoteDepth(projection, note.timeline, current_sample, note.sample);
                if (!z) {
                    continue;
                }
                const bool left =
                    note.button == sheets::SideButton::LeftUpper || note.button == sheets::SideButton::LeftLower;
                const double side = left ? -1.0 : 1.0;
                const ColorF color = NoteColor(note.timeline);
                if (note.button == sheets::SideButton::LeftUpper || note.button == sheets::SideButton::RightUpper) {
                    Box{ Vec3{ side * SideUpperX, SideUpperY, *z }, 0.24, 2.5, 0.6 }.draw(color);
                } else {
                    OrientedBox{ Vec3{ side * SideLowerX, 1.42, *z }, 2.8, 0.2, 0.6, SideSlope(side) }.draw(color);
                }
            }
        }

        void DrawChartNotes(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample
        ) {
            DrawSliderNotes(chart, projection, current_sample);
            DrawSideNotes(chart, projection, current_sample);
        }
    }

    PlayfieldDebug::PlayfieldDebug(const InitData& init)
        : SceneBase{ init }, m_camera{ DesignSize, 30_deg, InitialEye, InitialFocus },
          m_field_surface{ FieldSurfaceSize, TextureFormat::R8G8B8A8_Unorm_SRGB },
          m_render_texture{ DesignSize, TextureFormat::R8G8B8A8_Unorm_SRGB, HasDepth::Yes },
          m_chart{ MakeDebugChart() }, m_projection{ m_chart } {}

    void PlayfieldDebug::update() {
        if (KeyEscape.down()) {
            ClearPrint();
            changeScene(SceneState::Title, 0);
            return;
        }
        if (KeyR.down()) {
            m_camera.setView(InitialEye, InitialFocus);
        }
        if (KeySpace.down()) {
            m_paused = !m_paused;
        }
        if (KeyH.down()) {
            m_show_depth_guides = !m_show_depth_guides;
        }
        m_camera.update(2.0);
        if (!m_paused) {
            m_elapsed = std::fmod(m_elapsed + Scene::DeltaTime(), LoopDuration);
        }

        ClearPrint();
        Print << U"3D PLAYFIELD DEBUG  |  ESC: Title  R: Reset view  Space: Pause  H: Depth guides";
        Print << U"Camera: WASD move, E/X up/down, arrows look, Shift/Ctrl speed";
        Print << U"Eye: {}  Focus: {}"_fmt(m_camera.getEyePosition(), m_camera.getFocusPosition());
        Print << U"Chart: {:.2f} / {:.0f} sec  FOV: {:.1f} deg  {}"_fmt(
            m_elapsed,
            LoopDuration,
            Math::ToDegrees(m_camera.getVerticalFOV()),
            m_paused ? U"PAUSED" : U"PLAYING"
        );
        const Float3 near_corner = m_camera.worldToScreenPoint(Float3{ -FieldWidth * 0.5, 0.0, FieldNear });
        const Float3 far_corner = m_camera.worldToScreenPoint(Float3{ -FieldWidth * 0.5, 0.0, FieldFar });
        Print << U"Field left corners: near ({:.0f}, {:.0f}) [225,1080], far ({:.0f}, {:.0f}) [850,0]"_fmt(
            near_corner.x,
            near_corner.y,
            far_corner.x,
            far_corner.y
        );
        Print << U"Purple: 1.0x  Orange: 0.5x -> 1.5x  (same sample on all five regions)";
    }

    void PlayfieldDebug::draw() const {
        {
            const ScopedRenderTarget2D target{ m_field_surface.clear(ColorF{ 0.0, 0.0 }) };
            DrawFieldSurface(279);
            Graphics2D::Flush();
            m_field_surface.resolve();
        }

        Graphics3D::SetCameraTransform(m_camera);
        Graphics3D::SetGlobalAmbientColor(ColorF{ 1.0 });
        Graphics3D::SetSunColor(ColorF{ 0.0 });
        {
            const ScopedRenderTarget3D target{ m_render_texture.clear(ColorF{ U"#F7F8FC" }.removeSRGBCurve()) };
            DrawField(m_field_surface, m_show_depth_guides);
            DrawChartNotes(m_chart, m_projection, static_cast<int64>(m_elapsed * SampleRate));
        }
        Graphics3D::Flush();
        m_render_texture.resolve();
        Shader::LinearToScreen(m_render_texture);
    }
}
#endif
