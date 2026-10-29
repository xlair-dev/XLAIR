#pragma once

#include "Common.hpp"

namespace xlair::ui::presentation {
    [[nodiscard]]
    inline Optional<uint32>
    ResultRatingAt(const Optional<uint32>& previous_rating, const Optional<uint32>& rating, const double elapsed) {
        if (!rating || !previous_rating || previous_rating == rating) {
            return rating;
        }
        const double progress = elapsed > 1.0 ? Min((elapsed - 1.0) / 0.6, 1.0) : 0.0;
        const double value =
            Math::Lerp(static_cast<double>(*previous_rating), static_cast<double>(*rating), EaseOutCubic(progress));
        return static_cast<uint32>(Math::Round(value));
    }
}
