#include "PlayfieldDebug.hpp"
#include "ui/game/PlayfieldCamera.hpp"

#ifndef NDEBUG
#include <cmath>

namespace xlair::ui::scenes {
    namespace {
        constexpr double LoopDuration = 8.0;
        constexpr int64 SampleRate = 48'000;
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

            constexpr sheets::SliderNoteKind kinds[]{ sheets::SliderNoteKind::Tap,
                                                      sheets::SliderNoteKind::XTap,
                                                      sheets::SliderNoteKind::Flick };
            for (int32 index = 0; index < 4; ++index) {
                const int64 sample = (index + 1) * 3 * SampleRate / 2;
                const sheets::TimelineIndex timeline = index % 2;
                chart.slider_notes.push_back(
                    {
                        .kind = kinds[index % 3],
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
            chart.slider_holds.push_back({
                .points = {
                    { .kind = sheets::SliderHoldPointKind::Start,
                      .timeline = 0,
                      .sample = SampleRate,
                      .lane = { .start = 3, .width = 4 } },
                    { .kind = sheets::SliderHoldPointKind::Invisible,
                      .timeline = 0,
                      .sample = 2 * SampleRate,
                      .lane = { .start = 5, .width = 4 } },
                    { .kind = sheets::SliderHoldPointKind::Visible,
                      .timeline = 0,
                      .sample = 3 * SampleRate,
                      .lane = { .start = 7, .width = 4 } },
                    { .kind = sheets::SliderHoldPointKind::End,
                      .timeline = 0,
                      .sample = 5 * SampleRate,
                      .lane = { .start = 9, .width = 4 } },
                },
            });
            chart.side_holds.push_back({
                .button = sheets::SideButton::LeftLower,
                .points = {
                    { .kind = sheets::SideHoldPointKind::Start, .timeline = 0, .sample = 2 * SampleRate },
                    { .kind = sheets::SideHoldPointKind::Relay, .timeline = 0, .sample = 3 * SampleRate },
                    { .kind = sheets::SideHoldPointKind::End, .timeline = 0, .sample = 5 * SampleRate },
                },
            });
            chart.side_holds.push_back({
                .button = sheets::SideButton::RightUpper,
                .points = {
                    { .kind = sheets::SideHoldPointKind::Start, .timeline = 1, .sample = 3 * SampleRate },
                    { .kind = sheets::SideHoldPointKind::End, .timeline = 1, .sample = 6 * SampleRate },
                },
            });
            return chart;
        }
    }

    PlayfieldDebug::PlayfieldDebug(const InitData& init)
        : SceneBase{ init },
          m_camera{ DesignSize, game::PlayfieldCameraFOV, game::PlayfieldCameraEye, game::PlayfieldCameraFocus },
          m_chart{ MakeDebugChart() }, m_projection{ m_chart } {}

    void PlayfieldDebug::update() {
        if (KeyEscape.down()) {
            ClearPrint();
            changeScene(SceneState::Title, 0);
            return;
        }
        if (KeyR.down()) {
            m_camera.setView(game::PlayfieldCameraEye, game::PlayfieldCameraFocus);
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
        const Float3 near_corner = m_camera.worldToScreenPoint(
            Float3{ -game::PlayfieldRenderer3D::FieldWidth * 0.5, 0.0, game::PlayfieldRenderer3D::FieldNear }
        );
        const Float3 far_corner = m_camera.worldToScreenPoint(
            Float3{ -game::PlayfieldRenderer3D::FieldWidth * 0.5, 0.0, game::PlayfieldRenderer3D::FieldFar }
        );
        Print << U"Field left corners: near ({:.0f}, {:.0f}) [225,1080], far ({:.0f}, {:.0f}) [850,0]"_fmt(
            near_corner.x,
            near_corner.y,
            far_corner.x,
            far_corner.y
        );
        Print << U"Tap / XTap / Flick / Holds  |  Timelines: 1.0x and 0.5x -> 1.5x";
    }

    void PlayfieldDebug::draw() const {
        m_renderer.draw(
            m_camera,
            m_chart,
            m_projection,
            static_cast<int64>(m_elapsed * SampleRate),
            279,
            m_show_depth_guides
        );
    }
}
#endif
