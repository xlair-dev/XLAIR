#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    struct MenuTimerPlateData {
        int32 remaining_seconds = 0;
        uint32 max_plays = 0;
        uint32 remaining_plays = 0;
    };

    void DrawMenuTimerPlate(const MenuTimerPlateData& data, const Point& position);
}
