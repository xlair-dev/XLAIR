#pragma once

#include "Common.hpp"
#include "ui/game/PlayfieldRenderer3D.hpp"

namespace xlair::ui::components {
    class NoteSpeedPreview {
    public:
        NoteSpeedPreview();

        [[nodiscard]]
        static constexpr Size size() noexcept {
            return { 480, 270 };
        }

        void update(double delta_seconds);
        void draw(const RectF& region, double note_speed, double opacity = 1.0) const;

    private:
        BasicCamera3D m_camera;
        sheets::Chart m_chart;
        playfield::ChartProjection m_projection;
        game::PlayfieldRenderer3D m_renderer;
        mutable RenderTexture m_preview_texture;
        double m_elapsed = 0.0;
    };
}
