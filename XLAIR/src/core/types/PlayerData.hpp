#pragma once
#include "Common.hpp"

namespace core::types {
    struct PlayerData {
        String username;
        String displayname;
        int32 rating;
        int32 level;

        size_t selected_index = 0;
        int8 selected_difficulty = 0;
    };
}
