#pragma once

#include "Common.hpp"

#include <cmath>
#include <limits>

namespace xlair::ui::game {
    [[nodiscard]]
    inline int64 JudgmentOffsetSamples(const double offset_seconds, const int64 sample_rate) {
        const long double samples = static_cast<long double>(offset_seconds) * sample_rate;
        constexpr long double limit = static_cast<long double>(std::numeric_limits<int64>::max() / 2);
        return static_cast<int64>(std::round(Clamp(samples, -limit, limit)));
    }

    [[nodiscard]]
    inline int64 JudgedSample(const int64 sample, const int64 offset) {
        const long double adjusted = static_cast<long double>(sample) - offset;
        constexpr auto Minimum = static_cast<long double>(std::numeric_limits<int64>::min());
        constexpr auto Maximum = static_cast<long double>(std::numeric_limits<int64>::max());
        if (adjusted <= Minimum) {
            return std::numeric_limits<int64>::min();
        }
        if (adjusted >= Maximum) {
            return std::numeric_limits<int64>::max();
        }
        return static_cast<int64>(adjusted);
    }
}
