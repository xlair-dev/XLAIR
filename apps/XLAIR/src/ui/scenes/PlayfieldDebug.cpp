#include "PlayfieldDebug.hpp"

#ifndef NDEBUG
#include <cmath>

namespace xlair::ui::scenes {
    namespace {
        constexpr double FieldLength = 80.0;
        constexpr double FieldWidth = 16.0;
        constexpr double LowerWidth = 3.5355339059;
        constexpr Vec3 InitialEye{ 0.0, 12.0, -26.0 };
        constexpr Vec3 InitialFocus{ 0.0, 0.0, 30.0 };

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

        void DrawTestNotes(const double z) {
            const ColorF note_color = ColorF{ U"#D45CD7" }.removeSRGBCurve();
            Box{ Vec3{ 0.0, 0.18, z }, 3.0, 0.2, 0.6 }.draw(note_color);
            for (const double side : { -1.0, 1.0 }) {
                const Quaternion slope = Quaternion::RotateZ(side < 0.0 ? 135_deg : 45_deg);
                OrientedBox{ Vec3{ side * 9.25, 1.42, z }, 2.8, 0.2, 0.6, slope }.draw(note_color);
                Box{ Vec3{ side * 10.5, 4.0, z }, 0.24, 2.5, 0.6 }.draw(note_color);
            }
        }
    }

    PlayfieldDebug::PlayfieldDebug(const InitData& init)
        : SceneBase{ init }, m_camera{ DesignSize, 30_deg, InitialEye, InitialFocus },
          m_render_texture{ DesignSize, TextureFormat::R8G8B8A8_Unorm_SRGB, HasDepth::Yes } {}

    void PlayfieldDebug::update() {
        if (KeyEscape.down()) {
            ClearPrint();
            changeScene(SceneState::Title, 0);
            return;
        }
        if (KeyR.down()) {
            m_camera.setView(InitialEye, InitialFocus);
        }
        m_camera.update(2.0);
        m_elapsed += Scene::DeltaTime();

        ClearPrint();
        Print << U"3D PLAYFIELD DEBUG  |  ESC: Title  R: Reset view";
        Print << U"Camera: WASD move, E/X up/down, arrows look, Shift/Ctrl speed";
        Print << U"Eye: {}  Focus: {}"_fmt(m_camera.getEyePosition(), m_camera.getFocusPosition());
        Print << U"Note depth: {:.2f}  FOV: {:.1f} deg"_fmt(
            FieldLength - std::fmod(m_elapsed * 12.0, FieldLength),
            Math::ToDegrees(m_camera.getVerticalFOV())
        );
    }

    void PlayfieldDebug::draw() const {
        Graphics3D::SetCameraTransform(m_camera);
        Graphics3D::SetGlobalAmbientColor(ColorF{ 1.0 });
        Graphics3D::SetSunColor(ColorF{ 0.0 });
        {
            const ScopedRenderTarget3D target{ m_render_texture.clear(ColorF{ U"#F7F8FC" }.removeSRGBCurve()) };
            DrawField();
            DrawTestNotes(FieldLength - std::fmod(m_elapsed * 12.0, FieldLength));
        }
        Graphics3D::Flush();
        m_render_texture.resolve();
        Shader::LinearToScreen(m_render_texture);
    }
}
#endif
