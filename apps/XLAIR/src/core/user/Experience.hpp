#pragma once

#include "Common.hpp"

#include <limits>

namespace xlair::core::user {
    [[nodiscard]]
    constexpr uint32 ExperienceForScore(const uint32 score) noexcept {
        const uint32 bonus = score > 900'000 ? (score - 900'000) / 1'000 : 0;
        return Max(uint32{ 1 }, bonus);
    }

    [[nodiscard]]
    constexpr uint32 AddExperience(const uint32 current_xp, const uint32 gained_xp) noexcept {
        constexpr uint32 Maximum = std::numeric_limits<uint32>::max();
        return gained_xp > (Maximum - current_xp) ? Maximum : current_xp + gained_xp;
    }

    static_assert(ExperienceForScore(0) == 1);
    static_assert(ExperienceForScore(900'000) == 1);
    static_assert(ExperienceForScore(950'000) == 50);
    static_assert(ExperienceForScore(1'000'000) == 100);
    static_assert(ExperienceForScore(1'090'000) == 190);
    static_assert(AddExperience(std::numeric_limits<uint32>::max() - 1, 2) == std::numeric_limits<uint32>::max());
}
