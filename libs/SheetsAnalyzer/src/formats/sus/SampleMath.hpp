#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace xlair::sheets::formats::sus::detail {
    [[nodiscard]]
    inline std::int64_t RoundSample(const long double sample) {
        constexpr auto MinimumValue = std::numeric_limits<std::int64_t>::min();
        constexpr auto MaximumValue = std::numeric_limits<std::int64_t>::max();
        constexpr auto Minimum = static_cast<long double>(MinimumValue);
        constexpr auto MaximumExclusive = -Minimum;

        const long double rounded = std::round(sample);
        if (rounded <= Minimum) {
            return MinimumValue;
        }
        if (rounded >= MaximumExclusive) {
            return MaximumValue;
        }
        return static_cast<std::int64_t>(rounded);
    }
}
