#pragma once

#include "Common.hpp"

#include <Playfield/ChartProjection.hpp>
#include <SheetsAnalyzer/Chart.hpp>

namespace xlair::ui::game {
    class PlayfieldRenderer3D {
    public:
        static constexpr double FieldWidth = 16.0;
        static constexpr double FieldFar = 129.2447;
        // The judgement line is 100 / 4000 of the field length from the near edge.
        static constexpr double FieldNear = -FieldFar * 100.0 / 3900.0;

        PlayfieldRenderer3D();

        void draw(
            const BasicCamera3D& camera,
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            int64 current_sample,
            uint32 combo,
            bool show_depth_guides = false
        ) const;

    private:
        mutable MSRenderTexture m_field_surface;
        mutable MSRenderTexture m_render_texture;
    };
}
