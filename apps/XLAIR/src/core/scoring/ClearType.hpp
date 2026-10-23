#pragma once

#include "core/judgement/JudgeEngine.hpp"

namespace xlair::core::scoring {
    enum class ClearType {
        Failed,
        Clear,
        FullCombo,
        Perfect,
    };

    [[nodiscard]]
    constexpr ClearType ClearTypeForScore(const judgement::ScoreState& score, const std::size_t total_combo) noexcept {
        const uint64 judged_count =
            uint64{ score.perfect_count } + score.great_count + score.good_count + score.miss_count;
        if (total_combo == 0 || judged_count != total_combo) {
            return ClearType::Failed;
        }
        if (score.miss_count == 0) {
            return score.great_count == 0 && score.good_count == 0 ? ClearType::Perfect : ClearType::FullCombo;
        }
        return score.clear_gauge >= 0.7 ? ClearType::Clear : ClearType::Failed;
    }

    static_assert(ClearTypeForScore({}, 0) == ClearType::Failed);
    static_assert(ClearTypeForScore({}, 1) == ClearType::Failed);
    static_assert(ClearTypeForScore({ .perfect_count = 1 }, 1) == ClearType::Perfect);
    static_assert(ClearTypeForScore({ .great_count = 1 }, 1) == ClearType::FullCombo);
    static_assert(ClearTypeForScore({ .good_count = 1 }, 1) == ClearType::FullCombo);
    static_assert(ClearTypeForScore({ .miss_count = 1, .clear_gauge = 0.7 }, 1) == ClearType::Clear);
    static_assert(ClearTypeForScore({ .miss_count = 1, .clear_gauge = 0.69 }, 1) == ClearType::Failed);
}
