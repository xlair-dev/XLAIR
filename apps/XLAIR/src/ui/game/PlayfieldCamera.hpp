#pragma once

#include "Common.hpp"

namespace xlair::ui::game {
    // Game's fixed view; the 3D debug scene starts from the same framing.
    inline constexpr double PlayfieldCameraFOV = 30_deg;
    inline constexpr Vec3 PlayfieldCameraEye{ 0.0, 12.9974, -21.9358 };
    inline constexpr Vec3 PlayfieldCameraFocus{ 0.0, 0.0, 13.9422 };
}
