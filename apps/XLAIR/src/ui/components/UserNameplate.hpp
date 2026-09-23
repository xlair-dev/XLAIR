#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    struct UserNameplateData {
        StringView display_name;
        uint32 rating = 0;
        uint32 level = 0;
        double level_progress = 0.0;
    };

    void DrawUserNameplate(const UserNameplateData& data, const Point& position);
}
