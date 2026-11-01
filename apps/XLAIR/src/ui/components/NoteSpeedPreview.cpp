#include "NoteSpeedPreview.hpp"

#include "ui/Design.hpp"
#include "ui/game/PlayfieldCamera.hpp"
#include "ui/theme/Palette.hpp"

#include <cmath>

namespace xlair::ui::components {
    namespace {
        constexpr int64 SampleRate = 44'100;
        constexpr double NoteInterval = 1.0;
        constexpr Size ImageSize = NoteSpeedPreview::size();

        [[nodiscard]]
        sheets::Chart MakePreviewChart() {
            sheets::Chart chart;
            chart.sample_rate = SampleRate;
            chart.timelines.push_back({ .speed_changes = { { .sample = 0, .multiplier = 1.0 } } });
            // Cover the visible range at the minimum speed, including just-passed notes.
            for (int64 second = -2; second <= 40; ++second) {
                chart.slider_notes.push_back(
                    {
                        .kind = sheets::SliderNoteKind::Tap,
                        .sample = second * SampleRate,
                        .lane = { .start = 4, .width = 8 },
                    }
                );
            }
            chart.total_combo = chart.slider_notes.size();
            return chart;
        }
    }

    NoteSpeedPreview::NoteSpeedPreview()
        : m_camera{ DesignSize, game::PlayfieldCameraFOV, game::PlayfieldCameraEye, game::PlayfieldCameraFocus },
          m_chart{ MakePreviewChart() }, m_projection{ m_chart },
          m_preview_texture{ ImageSize, TextureFormat::R8G8B8A8_Unorm } {}

    void NoteSpeedPreview::update(const double delta_seconds) {
        if (std::isfinite(delta_seconds) && delta_seconds > 0.0) {
            m_elapsed = std::fmod(m_elapsed + std::fmod(delta_seconds, NoteInterval), NoteInterval);
        }
    }

    void NoteSpeedPreview::draw(const RectF& region, const double note_speed, const double opacity) const {
        if (!std::isfinite(note_speed) || note_speed <= 0.0 || !std::isfinite(opacity) || opacity <= 0.0 ||
            !std::isfinite(region.x) || !std::isfinite(region.y) || !std::isfinite(region.w) ||
            !std::isfinite(region.h) || !region.hasArea()) {
            return;
        }
        const auto& field = m_renderer.render(
            m_camera,
            m_chart,
            m_projection,
            static_cast<int64>(m_elapsed * SampleRate),
            Clamp(note_speed, 0.25, 10.0),
            0
        );
        {
            const ScopedRenderTarget2D target{ m_preview_texture.clear(ColorF{ 1.0 }) };
            Shader::LinearToScreen(field);
        }
        Graphics2D::Flush();
        const double scale = Min(region.w / ImageSize.x, region.h / ImageSize.y);
        const RectF destination{ Arg::center = region.center(), ImageSize * scale };
        const double alpha = Clamp(opacity, 0.0, 1.0);
        destination(m_preview_texture).draw(ColorF{ 1.0, alpha });
        destination.drawFrame(2.0 * scale, theme::Palette::Pink.withA(alpha));
    }
}
