#pragma once

#include "Common.hpp"

namespace xlair::core::user {
    // Temporary progression curve: advancing from level N requires N * 100 XP.
    // Keep this rule in one place so the curve can be replaced once the final balance is decided.
    inline constexpr uint32 ExperiencePerLevel = 100;

    struct LevelProgress {
        uint32 level = 1;
        uint32 xp_into_level = 0;
        uint32 xp_for_next_level = ExperiencePerLevel;
        double progress = 0.0;
    };

    [[nodiscard]]
    constexpr uint64 ExperienceToReachLevel(const uint32 level) noexcept {
        if (level <= 1) {
            return 0;
        }

        const uint64 completed_levels = static_cast<uint64>(level - 1);
        return ExperiencePerLevel * completed_levels * (completed_levels + 1) / 2;
    }

    [[nodiscard]]
    constexpr LevelProgress CalculateLevelProgress(const uint32 total_xp) noexcept {
        uint32 level = 1;
        uint64 xp_into_level = total_xp;

        while (true) {
            const uint64 xp_for_next_level = static_cast<uint64>(ExperiencePerLevel) * level;
            if (xp_into_level < xp_for_next_level) {
                return {
                    .level = level,
                    .xp_into_level = static_cast<uint32>(xp_into_level),
                    .xp_for_next_level = static_cast<uint32>(xp_for_next_level),
                    .progress = static_cast<double>(xp_into_level) / static_cast<double>(xp_for_next_level),
                };
            }

            xp_into_level -= xp_for_next_level;
            ++level;
        }
    }

    static_assert(ExperienceToReachLevel(1) == 0);
    static_assert(ExperienceToReachLevel(2) == 100);
    static_assert(ExperienceToReachLevel(3) == 300);

    static_assert(CalculateLevelProgress(0).level == 1);
    static_assert(CalculateLevelProgress(99).xp_into_level == 99);
    static_assert(CalculateLevelProgress(100).level == 2);
    static_assert(CalculateLevelProgress(299).xp_into_level == 199);
    static_assert(CalculateLevelProgress(300).level == 3);
}
