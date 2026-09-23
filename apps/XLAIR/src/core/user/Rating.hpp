#pragma once

#include "Common.hpp"

namespace xlair::core::user {
    struct RatingRecord {
        // The caller supplies one best-score record per sheet, matching the server's record model.
        double level = 0.0;
        uint32 score = 0;
        bool is_test = false;
    };

    [[nodiscard]]
    uint32 CalculateSheetRating(double level, uint32 score) noexcept;

    [[nodiscard]]
    uint32 CalculateUserRating(const Array<RatingRecord>& records);
}
