#pragma once

#include "Common.hpp"

#include <Playfield.hpp>
#include <SheetsAnalyzer.hpp>

namespace xlair::ui::game {
    class PlayfieldRenderer {
    public:
        PlayfieldRenderer();

        void draw(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            int64 current_sample,
            const Rect& viewport,
            double pixels_per_second,
            uint32 combo
        ) const;

    private:
        mutable MSRenderTexture m_field_texture;
    };
}
