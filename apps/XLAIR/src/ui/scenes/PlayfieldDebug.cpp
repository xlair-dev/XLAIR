#include "PlayfieldDebug.hpp"

#ifndef NDEBUG
#include <cmath>

namespace xlair::ui::scenes {
    namespace {
        constexpr double FieldLength = 80.0;
        constexpr double FieldWidth = 16.0;
        constexpr double LowerWidth = 3.5355339059;
        constexpr double WorldUnitsPerSecond = 12.0;
        constexpr double LoopDuration = 8.0;
        constexpr int64 SampleRate = 48'000;
        constexpr Vec3 InitialEye{ 0.0, 12.0, -26.0 };
        constexpr Vec3 InitialFocus{ 0.0, 0.0, 30.0 };

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

        void DrawField() {
            const ColorF floor_color = ColorF{ U"#212830" }.removeSRGBCurve();
            const ColorF lower_color = ColorF{ U"#FFFFFF" }.removeSRGBCurve();
            const ColorF upper_color = ColorF{ U"#D5F0FB" }.removeSRGBCurve();
            const ColorF guide_color = ColorF{ U"#89D4FF" }.removeSRGBCurve();

            Box{ Vec3{ 0.0, -0.06, FieldLength * 0.5 }, FieldWidth, 0.12, FieldLength }.draw(floor_color);
            for (const double side : { -1.0, 1.0 }) {
                const Quaternion slope = Quaternion::RotateZ(side < 0.0 ? 135_deg : 45_deg);
                OrientedBox{ Vec3{ side * 9.25, 1.25, FieldLength * 0.5 }, LowerWidth, 0.12, FieldLength, slope }.draw(
                    lower_color
                );
                Box{ Vec3{ side * 10.5, 4.0, FieldLength * 0.5 }, 0.12, 3.0, FieldLength }.draw(upper_color);
            }

            // Cross-section markers make it easy to see whether all regions share the same depth.
            for (double z = 0.0; z <= FieldLength; z += 20.0) {
                Box{ Vec3{ 0.0, 0.08, z }, FieldWidth, 0.08, 0.15 }.draw(guide_color);
                for (const double side : { -1.0, 1.0 }) {
                    const Quaternion slope = Quaternion::RotateZ(side < 0.0 ? 135_deg : 45_deg);
                    OrientedBox{ Vec3{ side * 9.25, 1.32, z }, LowerWidth, 0.08, 0.15, slope }.draw(guide_color);
                    Box{ Vec3{ side * 10.5, 4.0, z }, 0.16, 3.0, 0.15 }.draw(guide_color);
                }
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
            if (!std::isfinite(z) || z < -2.0 || z > FieldLength + 2.0) {
                return none;
            }
            return z;
        }

        [[nodiscard]]
        ColorF NoteColor(const sheets::TimelineIndex timeline) {
            return ColorF{ timeline == 0 ? U"#D45CD7" : U"#F5A623" }.removeSRGBCurve();
        }

        void DrawChartNotes(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample
        ) {
            for (const auto& note : chart.slider_notes) {
                const auto z = NoteDepth(projection, note.timeline, current_sample, note.sample);
                if (!z) {
                    continue;
                }
                const double x = -FieldWidth * 0.5 + note.lane.start + note.lane.width * 0.5;
                Box{ Vec3{ x, 0.18, *z }, note.lane.width * 0.9, 0.2, 0.6 }.draw(NoteColor(note.timeline));
            }
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
                    Box{ Vec3{ side * 10.5, 4.0, *z }, 0.24, 2.5, 0.6 }.draw(color);
                } else {
                    const Quaternion slope = Quaternion::RotateZ(left ? 135_deg : 45_deg);
                    OrientedBox{ Vec3{ side * 9.25, 1.42, *z }, 2.8, 0.2, 0.6, slope }.draw(color);
                }
            }
        }
    }

    PlayfieldDebug::PlayfieldDebug(const InitData& init)
        : SceneBase{ init }, m_camera{ DesignSize, 30_deg, InitialEye, InitialFocus },
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
        m_camera.update(2.0);
        if (!m_paused) {
            m_elapsed = std::fmod(m_elapsed + Scene::DeltaTime(), LoopDuration);
        }

        ClearPrint();
        Print << U"3D PLAYFIELD DEBUG  |  ESC: Title  R: Reset view  Space: Pause";
        Print << U"Camera: WASD move, E/X up/down, arrows look, Shift/Ctrl speed";
        Print << U"Eye: {}  Focus: {}"_fmt(m_camera.getEyePosition(), m_camera.getFocusPosition());
        Print << U"Chart: {:.2f} / {:.0f} sec  FOV: {:.1f} deg  {}"_fmt(
            m_elapsed,
            LoopDuration,
            Math::ToDegrees(m_camera.getVerticalFOV()),
            m_paused ? U"PAUSED" : U"PLAYING"
        );
        Print << U"Purple: 1.0x  Orange: 0.5x -> 1.5x  (same sample on all five regions)";
    }

    void PlayfieldDebug::draw() const {
        Graphics3D::SetCameraTransform(m_camera);
        Graphics3D::SetGlobalAmbientColor(ColorF{ 1.0 });
        Graphics3D::SetSunColor(ColorF{ 0.0 });
        {
            const ScopedRenderTarget3D target{ m_render_texture.clear(ColorF{ U"#F7F8FC" }.removeSRGBCurve()) };
            DrawField();
            DrawChartNotes(m_chart, m_projection, static_cast<int64>(m_elapsed * SampleRate));
        }
        Graphics3D::Flush();
        m_render_texture.resolve();
        Shader::LinearToScreen(m_render_texture);
    }
}
#endif
