#pragma once
#include <Common.hpp>
#include "core/types/Config.hpp"

namespace app::consts {
    namespace defaults {
        static inline constexpr int32 Width = 1920;
        static inline constexpr int32 Height = 1080;
        static inline constexpr bool Sizable = true;
        static inline constexpr bool Fullscreen = false;
        static inline constexpr ColorF LetterboxColor { 0 };

        static inline constexpr bool Arcade = false;
    }

    static inline constexpr core::types::Config DefaultConfig {
        .window = {
            .width = defaults::Width,
            .height = defaults::Height,
            .sizable = defaults::Sizable,
            .fullscreen = defaults::Fullscreen,
            .letterbox_color = defaults::LetterboxColor,
        },
        .system = {
            .arcade = defaults::Arcade,
        },
    };
}

