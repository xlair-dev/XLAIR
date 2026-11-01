#pragma once

#include "Common.hpp"
#include "app/flows/Tutorial.hpp"

#include <Playfield/ChartProjection.hpp>
#include <SheetsAnalyzer/Chart.hpp>

namespace xlair::ui::game {
    [[nodiscard]]
    Array<Quad> TutorialFocusRegions(
        const BasicCamera3D& camera,
        const sheets::Chart& chart,
        const playfield::ChartProjection& projection,
        const app::flows::Tutorial::Guide& guide,
        int64 music_sample,
        double note_speed
    );
}
