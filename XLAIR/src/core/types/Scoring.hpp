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

        double clear_guage = 0.0; // 0.0 to 1.0

        void updateScore(int32 total_combo) {
            if (total_combo <= 0) {
                score_value = 0;
                return;
            }
            const double score_tmp =
                1'000'000 * (
                    perfect_count * 1.1 +
                    great_count * 1.0 +
                    good_count * 0.4
                ) / static_cast<double>(total_combo);
            score_value = static_cast<int32>(score_tmp);
        }

        void updateClearGuage(int8 dir, int32 total_combo) {
            const auto delta = 5.0 / total_combo;
            if (dir > 0) {
                clear_guage += delta;
            } else if (dir < 0) {
                clear_guage -= delta;
            }
            clear_guage = Math::Clamp(clear_guage, 0.0, 1.0);
        }
    };

    inline String GetGrade(int32 score_value) {
        if (score_value >= 1'050'000) {
            return U"SSS";
        } else if (score_value >= 1'000'000) {
            return U"SS";
        } else if (score_value >= 950'000) {
            return U"S";
        } else if (score_value >= 900'000) {
            return U"AAA";
        } else if (score_value >= 850'000) {
            return U"AA";
        } else if (score_value >= 800'000) {
            return U"A";
        } else if (score_value >= 750'000) {
            return U"BBB";
        } else if (score_value >= 700'000) {
            return U"BB";
        } else {
            return U"B";
        }
    }

}
