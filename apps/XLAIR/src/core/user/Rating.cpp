#include "Rating.hpp"

#include <array>
#include <cmath>

namespace xlair::core::user {
    namespace {
        struct ScoreBonusAnchor {
            uint32 score;
            int32 bonus;
        };

        constexpr std::array ScoreBonusAnchors{
            ScoreBonusAnchor{ 700'000, -200 },  ScoreBonusAnchor{ 750'000, -150 },  ScoreBonusAnchor{ 800'000, -100 },
            ScoreBonusAnchor{ 850'000, -50 },   ScoreBonusAnchor{ 900'000, 0 },     ScoreBonusAnchor{ 950'000, 50 },
            ScoreBonusAnchor{ 1'000'000, 100 }, ScoreBonusAnchor{ 1'050'000, 150 }, ScoreBonusAnchor{ 1'090'000, 200 },
        };

        [[nodiscard]]
        constexpr int32 CalculateScoreBonus(const uint32 score) noexcept {
            if (score <= ScoreBonusAnchors.front().score) {
                return ScoreBonusAnchors.front().bonus;
            }
            if (score >= ScoreBonusAnchors.back().score) {
                return ScoreBonusAnchors.back().bonus;
            }

            for (size_t index = 1; index < ScoreBonusAnchors.size(); ++index) {
                const auto& lower = ScoreBonusAnchors[index - 1];
                const auto& upper = ScoreBonusAnchors[index];
                if (score > upper.score) {
                    continue;
                }

                const int64 score_range = upper.score - lower.score;
                const int64 score_position = score - lower.score;
                const int64 bonus_range = upper.bonus - lower.bonus;
                return static_cast<int32>(lower.bonus + bonus_range * score_position / score_range);
            }

            return ScoreBonusAnchors.front().bonus;
        }

        [[nodiscard]]
        constexpr uint32 CalculateSheetRatingFromTenths(const uint32 level_tenths, const uint32 score) noexcept {
            const int64 rating = static_cast<int64>(level_tenths) * 10 + CalculateScoreBonus(score);
            return rating > 0 ? static_cast<uint32>(rating) : 0;
        }

        static_assert(CalculateScoreBonus(700'000) == -200);
        static_assert(CalculateScoreBonus(725'000) == -175);
        static_assert(CalculateScoreBonus(900'000) == 0);
        static_assert(CalculateScoreBonus(1'025'000) == 125);
        static_assert(CalculateScoreBonus(1'090'000) == 200);
        static_assert(CalculateSheetRatingFromTenths(140, 1'000'000) == 1'500);
    }

    uint32 CalculateSheetRating(const double level, const uint32 score) noexcept {
        if (!std::isfinite(level) || level < 1.0 || level > 99.9) {
            return 0;
        }

        // The server stores sheet levels in tenths (for example, 13.7 as 137).
        const auto level_tenths = static_cast<uint32>(std::round(level * 10.0));
        return CalculateSheetRatingFromTenths(level_tenths, score);
    }

    uint32 CalculateUserRating(const Array<RatingRecord>& records) {
        constexpr size_t RatingSlots = 3;

        Array<uint32> sheet_ratings;
        sheet_ratings.reserve(records.size());
        for (const auto& record : records) {
            if (!record.is_test) {
                sheet_ratings.push_back(CalculateSheetRating(record.level, record.score));
            }
        }
        sheet_ratings.rsort();

        uint64 total = 0;
        for (size_t index = 0; index < Min(RatingSlots, sheet_ratings.size()); ++index) {
            total += sheet_ratings[index];
        }
        return static_cast<uint32>(total / RatingSlots);
    }
}
