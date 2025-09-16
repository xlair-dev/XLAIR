#pragma once
#include "Common.hpp"

namespace core::types {
    // TODO: Determine the difficulty name
    enum class Difficulty : uint8 {
        Basic,
        Advanced,
        Master,
        Unknown,
    };

    inline Difficulty DifficultyFromInt(const uint8 value) {
        switch (value) {
            case 0: return Difficulty::Basic;
            case 1: return Difficulty::Advanced;
            case 2: return Difficulty::Master;
            default: return Difficulty::Unknown;
        }
    }

    inline String ToString(Difficulty value) {
        switch (value) {
            case Difficulty::Basic: return U"Basic";
            case Difficulty::Advanced: return U"Advanced";
            case Difficulty::Master: return U"Master";
            default: return U"Unknown";
        }
    }
}