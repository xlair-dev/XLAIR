#pragma once
#include "Common.hpp"

namespace core::types {
    struct Score {
        int32 perfect_count = 0;
        int32 great_count = 0;
        int32 good_count = 0;
        int32 miss_count = 0;
        int32 max_combo = 0;
        int32 combo = 0;

        int32 score_value = 0;
        int32 prev_score_value = 0;
        double last_update_time = 0.0;

        //void updateScore() {}
    };
}
