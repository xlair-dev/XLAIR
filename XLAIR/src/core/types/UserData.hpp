#pragma once
#include "Common.hpp"

namespace core::types {
    struct UserData {
        String id;
        String card;
        String displayname;
        int32 rating = 0;
        int32 xp = 0;
        int32 level = 0;
        int32 credits = 0;
        bool isPublic = false;
        bool isAdmin = false;

        size_t selected_index = 0;
        int8 selected_difficulty = 0;
    };
}
