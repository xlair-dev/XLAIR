#pragma once

#include <algorithm>
#include <cmath>

namespace xlair::ui::presentation {
    inline constexpr double TutorialStartSeconds = 0.65;

    struct TutorialStartPresentation {
        bool active;
        double prompt_opacity;
        double prompt_exit;
        double field_opacity;
    };

    [[nodiscard]]
    inline TutorialStartPresentation MakeTutorialStart(const double elapsed_seconds) noexcept {
        constexpr double PromptExitSeconds = 0.22;
        constexpr double FieldEnterSeconds = 0.14;
        const double elapsed = std::isfinite(elapsed_seconds) ? std::max(0.0, elapsed_seconds) : 0.0;
        const auto ease_out_cubic = [](const double progress) noexcept {
            const double remaining = 1.0 - std::clamp(progress, 0.0, 1.0);
            return 1.0 - remaining * remaining * remaining;
        };
        const double prompt_exit = ease_out_cubic(elapsed / PromptExitSeconds);
        return {
            .active = elapsed < TutorialStartSeconds,
            .prompt_opacity = 1.0 - prompt_exit,
            .prompt_exit = prompt_exit,
            .field_opacity = ease_out_cubic((elapsed - FieldEnterSeconds) / (TutorialStartSeconds - FieldEnterSeconds)),
        };
    }
}
