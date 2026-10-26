#pragma once

#include "Common.hpp"

namespace xlair::ui::audio {
    // A silent lead-in followed by a looping click, aligned to the calibration timeline.
    [[nodiscard]]
    Audio CreateCalibrationTrack();
}
