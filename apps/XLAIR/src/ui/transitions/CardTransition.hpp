#pragma once

#include "Common.hpp"

namespace xlair::ui::transitions {
    enum class CardZoom {
        Expand,
        Shrink,
    };

    struct CardTransition {
        double scale = 1.0;
        double opacity = 1.0;
    };

    // A non-crossfade transition splits this duration equally between the two scenes.
    inline constexpr int32 CardTransitionMillisec = 600;

    [[nodiscard]]
    inline CardTransition CardFadeIn(const double t, const CardZoom zoom = CardZoom::Expand) {
        const double progress = EaseOutQuart(Clamp(t, 0.0, 1.0));
        return {
            .scale = zoom == CardZoom::Expand ? progress : 2.0 - progress,
            .opacity = progress,
        };
    }

    [[nodiscard]]
    inline CardTransition CardFadeOut(const double t, const CardZoom zoom = CardZoom::Expand) {
        const double progress = EaseOutQuart(Clamp(t, 0.0, 1.0));
        return {
            .scale = zoom == CardZoom::Expand ? 1.0 + progress : 1.0 - progress,
            .opacity = 1.0 - progress,
        };
    }
}
