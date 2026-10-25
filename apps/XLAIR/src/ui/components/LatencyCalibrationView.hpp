#pragma once

#include "app/flows/LatencyCalibration.hpp"

namespace xlair::ui::components {
    struct LatencyCalibrationView {
        const app::flows::LatencyCalibration& calibration;
        double time_ms = 0.0;
        Optional<double> applied_offset_ms;
        bool audio_playing = false;
        bool applied = false;
    };

    void DrawLatencyCalibration(const LatencyCalibrationView& view, const Size& viewport);
}
