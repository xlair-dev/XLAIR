#pragma once

#include "Common.hpp"

namespace xlair::core::scoring {
    enum class Grade {
        B,
        BB,
        BBB,
        A,
        AA,
        AAA,
        S,
        SS,
        SSS,
    };

    [[nodiscard]]
    constexpr Grade GradeForScore(const uint32 score) noexcept {
        if (score >= 1'050'000) {
            return Grade::SSS;
        }
        if (score >= 1'000'000) {
            return Grade::SS;
        }
        if (score >= 950'000) {
            return Grade::S;
        }
        if (score >= 900'000) {
            return Grade::AAA;
        }
        if (score >= 850'000) {
            return Grade::AA;
        }
        if (score >= 800'000) {
            return Grade::A;
        }
        if (score >= 750'000) {
            return Grade::BBB;
        }
        if (score >= 700'000) {
            return Grade::BB;
        }
        return Grade::B;
    }

    static_assert(GradeForScore(699'999) == Grade::B);
    static_assert(GradeForScore(700'000) == Grade::BB);
    static_assert(GradeForScore(1'050'000) == Grade::SSS);
}
