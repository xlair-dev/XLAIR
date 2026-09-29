#pragma once

#include "Common.hpp"
#include "core/scoring/Grade.hpp"

#include <ApiClient/Types.hpp>

namespace xlair::ui::presentation {
    [[nodiscard]]
    constexpr StringView GradeLabel(const core::scoring::Grade grade) noexcept {
        switch (grade) {
            case core::scoring::Grade::B:
                return U"B";
            case core::scoring::Grade::BB:
                return U"BB";
            case core::scoring::Grade::BBB:
                return U"BBB";
            case core::scoring::Grade::A:
                return U"A";
            case core::scoring::Grade::AA:
                return U"AA";
            case core::scoring::Grade::AAA:
                return U"AAA";
            case core::scoring::Grade::S:
                return U"S";
            case core::scoring::Grade::SS:
                return U"SS";
            case core::scoring::Grade::SSS:
                return U"SSS";
        }
        return U"";
    }

    [[nodiscard]]
    constexpr StringView ClearStatusLabel(const api::ClearType clear_type) noexcept {
        switch (clear_type) {
            case api::ClearType::Clear:
                return U"CLEAR";
            case api::ClearType::FullCombo:
                return U"FULL COMBO";
            case api::ClearType::Perfect:
                return U"PERFECT";
            case api::ClearType::Failed:
                return U"";
        }
        return U"";
    }
}
