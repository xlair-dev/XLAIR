#pragma once
#include "Common.hpp"

namespace core::types {
    enum class ClearType : uint8 {
        failed,
        clear,
        fullcombo,
        perfect,
    };

    inline String ClearTypeToString(ClearType value) {
        switch (value) {
            case ClearType::failed: return U"FAILED";
            case ClearType::clear: return U"CLEAR";
            case ClearType::fullcombo: return U"FULL COMBO";
            case ClearType::perfect: return U"PERFECT";
            default: return U"unknown";
        }
    }

    struct Record {
        String id;
        String sheet_id;
        int32 score = 0;
        ClearType clear_type = ClearType::failed;
        int32 play_count = 0;
    };
}
