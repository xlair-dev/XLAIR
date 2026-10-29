#pragma once

#include "Common.hpp"

namespace xlair::ui::transitions {
    inline constexpr int32 ScreenWipeMillisec = 1200;

    struct ScreenWipeGeometry {
        Quad background;
        Quad accent;
    };

    [[nodiscard]]
    ScreenWipeGeometry MakeScreenWipe(double progress, const SizeF& size);

    void DrawScreenWipe(double progress);
}
