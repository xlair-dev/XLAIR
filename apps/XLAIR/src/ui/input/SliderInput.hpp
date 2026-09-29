#pragma once

#include "app/controller/Controller.hpp"

namespace xlair::ui::input {
    [[nodiscard]]
    inline bool TouchRegionDown(const app::controller::Controller* controller, const uint32 start, const uint32 width) {
        constexpr uint32 ColumnCount = app::controller::TouchZoneCount / 2;
        if (!controller || start >= ColumnCount || width == 0) {
            return false;
        }

        const std::size_t first_zone = static_cast<std::size_t>(start) * 2;
        const std::size_t last_zone = static_cast<std::size_t>(start + Min(width, ColumnCount - start)) * 2;
        for (std::size_t zone = first_zone; zone < last_zone; ++zone) {
            if (controller->touchZone(zone).down()) {
                return true;
            }
        }
        return false;
    }
}
